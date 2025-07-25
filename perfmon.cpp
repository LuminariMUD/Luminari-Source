/**
 * @file perfmon.cpp
 * @brief Implementation of Performance Monitoring System
 *
 * This file implements a comprehensive performance monitoring system for game servers.
 * The system tracks two main types of performance data:
 *
 * 1. Pulse Performance: Monitors the main game loop timing and maintains hierarchical
 *    statistics across different time intervals (pulse -> second -> minute -> hour -> day)
 *
 * 2. Code Profiling: Tracks execution time of specific code sections to identify
 *    performance bottlenecks
 *
 * The implementation uses a hierarchical data aggregation system where detailed
 * pulse-level data is automatically rolled up into higher-level time intervals.
 */

// Standard C++ library includes for data structures and I/O
#include <map>          // For storing profiling sections by name
#include <string>       // For string handling
#include <sstream>      // For formatted string output
#include <iomanip>      // For output formatting (precision, width, etc.)
#include <vector>       // For storing time-series data
#include <ctime>        // For time calculations
#include <cfloat>       // For DBL_MAX constant
#include <cstring>      // For string operations

// C library includes wrapped in extern "C" for C++ compatibility
extern "C" {
#include <sys/time.h>   // For gettimeofday() and timeval operations
#include "perfmon.h"    // Our own header file
} // extern "C"

/* ========================================================================
 * CONSTANTS AND MACROS
 * ======================================================================== */

/**
 * @brief Represents an infinite value for minimum calculations
 *
 * Used as initial value when calculating minimum performance values.
 * When no data has been recorded yet, the minimum is set to "infinity"
 * so that any real value will be smaller.
 */
static double const INFINITY = DBL_MAX;

// Time conversion constants - these create a hierarchy of time intervals
#define PULSE_PER_SECOND (PERF_pulse_per_second)                    ///< Pulses in one second
#define SEC_PER_MIN 60                                              ///< Seconds in one minute
#define PULSE_PER_MIN (PULSE_PER_SECOND * SEC_PER_MIN)             ///< Pulses in one minute
#define MIN_PER_HOUR 60                                             ///< Minutes in one hour
#define PULSE_PER_HOUR (PULSE_PER_MIN * MIN_PER_HOUR)              ///< Pulses in one hour
#define HOUR_PER_DAY 24                                             ///< Hours in one day
#define PULSE_PER_DAY (PULSE_PER_HOUR * HOUR_PER_DAY)              ///< Pulses in one day

/**
 * @brief Microseconds per pulse - maximum time allowed for one game loop iteration
 *
 * This represents the "budget" for each pulse. If a pulse takes longer than this,
 * it's running over its allocated time slice. For example, if PULSE_PER_SECOND = 10,
 * then USEC_PER_PULSE = 100,000 microseconds (100ms) per pulse.
 */
#define USEC_PER_PULSE (1000000 / PULSE_PER_SECOND)

/**
 * @brief Convert a timeval structure to total microseconds
 *
 * This macro converts a struct timeval (which has separate seconds and microseconds
 * fields) into a single microsecond value for easier arithmetic operations.
 *
 * @param val Pointer to struct timeval
 * @return Total microseconds as a long integer
 */
#define USEC_TOTAL( val ) ( ( (val)->tv_sec * 1000000) + (val)->tv_usec )


/* ========================================================================
 * PERFORMANCE THRESHOLD TRACKING
 * ======================================================================== */

/**
 * @brief Performance threshold tracking structure
 *
 * This array defines performance thresholds and tracks how often the server
 * exceeds each threshold. Each entry contains:
 * - threshold: Performance percentage threshold (e.g., 100% = one full pulse time)
 * - count: Number of times performance exceeded this threshold
 *
 * The thresholds help identify performance problems:
 * - 10-90%: Normal operation ranges
 * - 100%: Pulse took exactly its allocated time (warning level)
 * - 250%+: Severe performance problems (pulse took 2.5x+ longer than allowed)
 */
static struct
{
    int const threshold;        ///< Performance threshold percentage
    unsigned long count;        ///< Number of times this threshold was exceeded
} threshold_info [] =
{
    /* IMPORTANT: Must be in ascending order for check_thresholds() to work correctly */
    {10,    0},     ///< Light load threshold
    {30,    0},     ///< Moderate load threshold
    {50,    0},     ///< Heavy load threshold
    {70,    0},     ///< Very heavy load threshold
    {90,    0},     ///< Near-capacity threshold
    {100,   0},     ///< At-capacity threshold (pulse used full time slice)
    {250,   0},     ///< Severe overload (2.5x over budget)
    {500,   0},     ///< Critical overload (5x over budget)
    {1000,  0},     ///< Extreme overload (10x over budget)
    {2500,  0}      ///< Catastrophic overload (25x over budget)
};

/* ========================================================================
 * GLOBAL STATE VARIABLES
 * ======================================================================== */

/**
 * @brief Server initialization timestamp
 *
 * Records when the performance monitoring system was first initialized.
 * Used to calculate total server uptime and compute cumulative performance
 * percentages in reports.
 */
static time_t init_time;

/**
 * @brief Most recent pulse performance value
 *
 * Stores the performance percentage of the last recorded pulse.
 * Used in performance reports to show current/immediate performance.
 */
static double last_pulse;

/**
 * @brief Maximum pulse performance ever recorded
 *
 * Tracks the worst (highest) pulse performance percentage since server startup.
 * This helps identify the peak load the server has experienced.
 * Values over 100% indicate the server fell behind its target timing.
 */
static double max_pulse = 0;


/* ========================================================================
 * HIERARCHICAL PERFORMANCE DATA STORAGE
 * ======================================================================== */

/**
 * @brief Circular buffer for storing performance data at different time intervals
 *
 * This class implements a circular buffer that stores performance statistics
 * (average, minimum, maximum) for a specific time interval. When the buffer
 * fills up, it automatically aggregates its data and passes it to the next
 * higher-level interval.
 *
 * The hierarchical structure works like this:
 * - Pulse data (individual game loop iterations)
 * - Second data (aggregated from pulse data)
 * - Minute data (aggregated from second data)
 * - Hour data (aggregated from minute data)
 * - Day data (aggregated from hour data)
 *
 * This allows the system to maintain detailed recent history while keeping
 * long-term trends without consuming excessive memory.
 */
class PerfIntvlData
{
public:
    /**
     * @brief Constructor for performance interval data storage
     *
     * @param size Number of data points to store in this interval (buffer size)
     * @param next Pointer to the next higher-level interval (for data aggregation)
     *             Can be NULL if this is the highest level (day data)
     *
     * Example: For second-level data with 60 entries, when the 60th second
     * completes, all 60 seconds of data get aggregated into one minute entry
     * in the next higher level.
     */
    explicit PerfIntvlData( size_t size, PerfIntvlData *next )
        : mSize( size )         // Maximum number of entries in this circular buffer
        , mInd( 0 )            // Current write position in the circular buffer
        , mCount( 0 )          // Number of valid entries (grows until buffer is full)
        , mAvgs( size )        // Vector storing average values for each time slot
        , mMins( size )        // Vector storing minimum values for each time slot
        , mMaxes( size )       // Vector storing maximum values for each time slot
        , mpNextIntvl( next )  // Pointer to next higher-level interval for aggregation
    {
        // Constructor body is empty - all initialization done in member initializer list
    }

    /**
     * @brief Add new performance data to this interval
     *
     * Stores the provided performance statistics in the circular buffer.
     * When the buffer wraps around (becomes full), automatically aggregates
     * all current data and passes it to the next higher-level interval.
     *
     * @param avg Average performance value for this time slot
     * @param min Minimum performance value for this time slot
     * @param max Maximum performance value for this time slot
     */
    void AddData(double avg, double min, double max);

    /**
     * @brief Calculate average of all average values in this interval
     * @return Overall average performance across all stored time slots
     */
    double GetAvgAvg() const;

    /**
     * @brief Find the minimum of all minimum values in this interval
     * @return The lowest minimum performance value across all stored time slots
     */
    double GetMinMin() const;

    /**
     * @brief Find the maximum of all maximum values in this interval
     * @return The highest maximum performance value across all stored time slots
     */
    double GetMaxMax() const;

    /**
     * @brief Get the number of valid data entries currently stored
     * @return Number of time slots that contain valid data (0 to mSize)
     */
    size_t GetCount() const { return mCount; }

private:
    // Prevent copying - these objects manage resources and should not be copied
    PerfIntvlData( const PerfIntvlData & ); // unimplemented
    PerfIntvlData & operator=( const PerfIntvlData & ); // unimplemented

    size_t mSize;                    ///< Maximum capacity of the circular buffer
    size_t mInd;                     ///< Current write index (0 to mSize-1)
    size_t mCount;                   ///< Number of valid entries (0 to mSize)

    std::vector<double> mAvgs;       ///< Circular buffer of average values
    std::vector<double> mMins;       ///< Circular buffer of minimum values
    std::vector<double> mMaxes;      ///< Circular buffer of maximum values

    PerfIntvlData *mpNextIntvl;      ///< Pointer to next higher-level interval
};

/* ========================================================================
 * GLOBAL PERFORMANCE DATA HIERARCHY
 * ======================================================================== */

/**
 * @brief Global instances of performance data storage for different time intervals
 *
 * These static instances form a hierarchy where data flows from detailed (pulse-level)
 * to summarized (day-level) as time progresses. The hierarchy is:
 *
 * sPulseData -> sSecData -> sMinuteData -> sHourData -> (no next level)
 *
 * Each level automatically aggregates its data and passes it up when full.
 */

/// Stores 24 hours worth of hourly performance data (highest level - no next level)
static PerfIntvlData sHourData( HOUR_PER_DAY, NULL );

/// Stores 60 minutes worth of per-minute performance data (feeds into sHourData)
static PerfIntvlData sMinuteData( MIN_PER_HOUR, &sHourData );

/// Stores 60 seconds worth of per-second performance data (feeds into sMinuteData)
static PerfIntvlData sSecData( SEC_PER_MIN, &sMinuteData );

/// Stores individual pulse performance data (feeds into sSecData)
/// Buffer size = PULSE_PER_SECOND (e.g., 10 pulses if server runs at 10 Hz)
static PerfIntvlData sPulseData( PULSE_PER_SECOND, &sSecData );

/* ========================================================================
 * PERFORMANCE INTERVAL DATA METHODS
 * ======================================================================== */

/**
 * @brief Add new performance data to this interval's circular buffer
 *
 * This method implements the core data aggregation logic. It stores the new
 * data point in the circular buffer and handles buffer wraparound by
 * aggregating all current data and passing it to the next higher level.
 *
 * @param avg Average performance value for this time slot
 * @param min Minimum performance value for this time slot
 * @param max Maximum performance value for this time slot
 */
void
PerfIntvlData::AddData(double avg, double min, double max)
{
  // Store the new data at the current write position
  mAvgs[mInd] = avg;
  mMins[mInd] = min;
  mMaxes[mInd] = max;

  // Update the count of valid entries (grows until buffer is full)
  if ( mCount <= mInd )
  {
    mCount = mInd + 1;
  }

  // Advance to the next write position
  ++mInd;  // Pre-increment is slightly more efficient

  if ( mInd >= mSize )
  {
    // Buffer is full - wrap around to beginning
    mInd = 0;

    // Aggregate all our data and pass it to the next higher level
    if ( mpNextIntvl )
    {
      mpNextIntvl->AddData(
          this->GetAvgAvg(),    // Average of all our averages
          this->GetMinMin(),    // Minimum of all our minimums
          this->GetMaxMax());   // Maximum of all our maximums
    }
  }
}

/**
 * @brief Calculate the average of all average values stored in this interval
 *
 * This method computes the overall average performance across all time slots
 * in this interval. For example, if this is a minute-level interval containing
 * 60 seconds of data, this returns the average performance across those 60 seconds.
 *
 * @return Average of all average values, or 0 if no data is stored
 */
double
PerfIntvlData::GetAvgAvg() const
{
  // Early return for empty data
  if (mCount == 0)
  {
    return 0.0;
  }

  double sum = 0.0;

  // Sum all the average values we have stored
  for ( size_t i = 0 ; i < mCount ; ++i)
  {
    sum += mAvgs[i];
  }

  // Return the average of the averages
  return sum / static_cast<double>(mCount);
}

/**
 * @brief Find the minimum of all minimum values stored in this interval
 *
 * This method finds the lowest minimum performance value across all time slots.
 * This represents the best (lowest) performance spike during this interval.
 *
 * @return The smallest minimum value, or INFINITY if no data is stored
 */
double
PerfIntvlData::GetMinMin() const
{
  // Early return for empty data
  if (mCount == 0)
  {
    return INFINITY;
  }

  double min = mMins[0];  // Start with first value instead of INFINITY

  // Find the smallest minimum value across all time slots
  for ( size_t i = 1 ; i < mCount ; ++i )
  {
    if ( mMins[i] < min )
    {
      min = mMins[i];
    }
  }

  return min;
}

/**
 * @brief Find the maximum of all maximum values stored in this interval
 *
 * This method finds the highest maximum performance value across all time slots.
 * This represents the worst (highest) performance spike during this interval.
 *
 * @return The largest maximum value, or 0 if no data is stored
 */
double
PerfIntvlData::GetMaxMax() const
{
  // Early return for empty data
  if (mCount == 0)
  {
    return 0.0;
  }

  double max = mMaxes[0];  // Start with first value instead of 0

  // Find the largest maximum value across all time slots
  for ( size_t i = 1 ; i < mCount ; ++i )
  {
    if ( mMaxes[i] > max )
    {
      max = mMaxes[i];
    }
  }

  return max;
}

/* ========================================================================
 * UTILITY FUNCTIONS
 * ======================================================================== */

/**
 * @brief Initialize the performance monitoring system (called automatically)
 *
 * This function performs one-time initialization of the performance monitoring
 * system. It records the current time as the system start time, which is used
 * later to calculate total uptime and cumulative performance percentages.
 *
 * The function uses a static flag to ensure initialization happens only once,
 * even if called multiple times.
 */
static void check_init( void )
{
  static bool init_done = false;  // Static flag to prevent re-initialization
  if (init_done)
    return;

  // Record the current time as system initialization time
  init_time = time(NULL);

  init_done = true;
}

/**
 * @brief Update threshold violation counters based on performance value
 *
 * This function checks a performance value against all defined thresholds
 * and increments the counter for each threshold that was exceeded. This
 * creates statistics about how often the server performs poorly.
 *
 * The threshold_info array must be sorted in ascending order for this
 * function to work correctly (it stops checking once it finds a threshold
 * that wasn't exceeded).
 *
 * @param val Performance value as percentage (e.g., 150.0 = 150% of target time)
 */
static void check_thresholds(double val)
{
  const unsigned int thresh_count = sizeof(threshold_info) / sizeof(threshold_info[0]);

  unsigned int i;

  // Check each threshold in ascending order
  for (i = 0; i < thresh_count; ++i)
  {
    if (val > threshold_info[i].threshold)
    {
      // Performance exceeded this threshold - increment its counter
      ++(threshold_info[i].count);
    }
    else
    {
      // Performance didn't exceed this threshold, and since the array
      // is sorted in ascending order, it won't exceed any higher thresholds either
      break;
    }
  }
}

/* ========================================================================
 * PUBLIC API FUNCTIONS - PULSE MONITORING
 * ======================================================================== */

/**
 * @brief Log the performance of a single game loop pulse
 *
 * This is the main entry point for recording pulse performance data. It:
 * 1. Ensures the system is initialized
 * 2. Updates current and maximum pulse tracking
 * 3. Checks for threshold violations
 * 4. Adds the data to the hierarchical storage system
 *
 * The performance value represents how much of the allocated time slice
 * was used. Values over 100% indicate the pulse took longer than its
 * allocated time, which can cause the server to fall behind.
 *
 * @param val Performance as percentage of allocated time (e.g., 85.5 = 85.5%)
 *            Values over 100% indicate performance problems
 */
void PERF_log_pulse(double val)
{
  // Ensure the monitoring system is initialized
  check_init();

  // Update current pulse tracking
  last_pulse = val;

  // Update maximum pulse tracking (worst performance ever seen)
  if (val > max_pulse)
    max_pulse = val;

  // Update threshold violation counters
  check_thresholds(val);

  // Add this pulse data to the hierarchical storage system
  // For pulse data, avg=min=max since it's a single data point
  sPulseData.AddData( val, val, val );
}

/**
 * @brief Generate a comprehensive performance report
 *
 * This function creates a detailed text report showing performance statistics
 * across all time intervals (pulse, second, minute, hour) and threshold
 * violation statistics. The report is formatted for easy reading by administrators.
 *
 * The report includes:
 * - Current pulse performance and statistics for recent time periods
 * - Average, minimum, and maximum performance for each time interval
 * - Maximum pulse time ever recorded since server startup
 * - Percentage of time spent above various performance thresholds
 *
 * @param out_buf Buffer to store the formatted report string
 * @param n Size of the output buffer (including space for null terminator)
 * @return Number of characters written to the buffer (excluding null terminator)
 */
size_t PERF_repr( char *out_buf, size_t n )
{
  // Input validation - ensure we have a valid output buffer
  if (!out_buf)
  {
    return 0;
  }
  if (n < 1)
  {
    out_buf[0] = '\0';
    return 0;
  }

  // Use string stream for formatted output with reserve for better performance
  std::ostringstream os;
  os.str().reserve(1024);  // Reserve space to reduce reallocations

    // Calculate basic statistics for the report
    const unsigned int thresh_count = sizeof(threshold_info) / sizeof(threshold_info[0]);
    unsigned int i;
    time_t total_secs = time(NULL) - init_time;  // Total server uptime in seconds
    double total_pulses = total_secs * PULSE_PER_SECOND;  // Total pulses since startup

    // Get minimum values for each time interval, converting INFINITY to 0 for display
    // (INFINITY is used internally when no data has been recorded yet)
    double pulse_min = sPulseData.GetMinMin();
    pulse_min = (pulse_min == INFINITY) ? 0 : pulse_min;

    double sec_min = sSecData.GetMinMin();
    sec_min = (sec_min == INFINITY) ? 0 : sec_min;

    double min_min = sMinuteData.GetMinMin();
    min_min = (min_min == INFINITY) ? 0 : min_min;

    double hour_min = sHourData.GetMinMin();
    hour_min = (hour_min == INFINITY) ? 0 : hour_min;

    // Format the main performance statistics table
    // Use fixed-point notation with 2 decimal places for consistent formatting
    os << std::fixed << std::setprecision(2)
       << "                     Avg         Min         Max\n\r"

       // Current pulse (most recent single pulse)
       << std::setw(3) << 1 << " Pulse:   "
       << std::setw(10) << last_pulse << "% "
       << std::setw(10) << last_pulse << "% "
       << std::setw(10) << last_pulse << "%\n\r"

       // Recent pulses (up to PULSE_PER_SECOND pulses)
       << std::setw(3) << sPulseData.GetCount() << " Pulses:  "
       << std::setw(10) << sPulseData.GetAvgAvg() << "% "
       << std::setw(10) << pulse_min << "% "
       << std::setw(10) << sPulseData.GetMaxMax() << "%\n\r"

       // Recent seconds (up to 60 seconds)
       << std::setw(3) << sSecData.GetCount() << " Seconds: "
       << std::setw(10) << sSecData.GetAvgAvg() << "% "
       << std::setw(10) << sec_min << "% "
       << std::setw(10) << sSecData.GetMaxMax() << "%\n\r"

       // Recent minutes (up to 60 minutes)
       << std::setw(3) << sMinuteData.GetCount() << " Minutes: "
       << std::setw(10) << sMinuteData.GetAvgAvg() << "% "
       << std::setw(10) << min_min << "% "
       << std::setw(10) << sMinuteData.GetMaxMax() << "%\n\r"

       // Recent hours (up to 24 hours)
       << std::setw(3) << sHourData.GetCount() << " Hours:   "
       << std::setw(10) << sHourData.GetAvgAvg() << "% "
       << std::setw(10) << hour_min << "% "
       << std::setw(10) << sHourData.GetMaxMax() << "%\n\r"

       // Maximum pulse ever recorded (worst performance spike)
       << "\n\rMax pulse:      " << max_pulse << "\n\r\n\r";

    // Generate threshold violation statistics
    // This shows what percentage of time the server spent above each performance threshold
    for (i=0; i < thresh_count; ++i)
    {
        os << "Over " << std::setw(5)
           << threshold_info[i].threshold << "%:      ";

        // Calculate percentage of total runtime spent above this threshold
        // Protect against division by zero
        if (total_pulses > 0.0)
        {
            double percentage = (static_cast<double>(threshold_info[i].count) / total_pulses) * 100.0;
            os << std::setprecision(2) << std::fixed << percentage << "% ";
        }
        else
        {
            os << "0.00% ";
        }

        // Show raw count in parentheses
        os << "(" << threshold_info[i].count << ")\n\r";
    }

    // Convert the formatted output to a C-style string
    std::string str = os.str();

    // Ensure we don't exceed buffer bounds
    size_t max_copy = (n > 0) ? n - 1 : 0;  // Leave room for null terminator
    size_t to_copy = std::min(str.length(), max_copy);

    if (to_copy > 0)
    {
        str.copy( out_buf, to_copy );
    }

    out_buf[to_copy] = '\0';  // Ensure null termination

    return to_copy;
}

/* ========================================================================
 * CODE PROFILING SECTION CLASS
 * ======================================================================== */

/**
 * @brief Represents a single profiled code section with timing statistics
 *
 * This class tracks detailed timing information for a specific named section
 * of code. It maintains two sets of statistics:
 *
 * 1. Per-pulse statistics: Reset at the beginning of each game loop iteration
 *    - Used to see what happened during the current pulse
 *    - Helps identify which sections are consuming time in the current frame
 *
 * 2. Cumulative statistics: Accumulated since server startup
 *    - Used to see long-term performance trends
 *    - Helps identify which sections consume the most total time
 *
 * The class uses high-precision timeval structures (microsecond resolution)
 * to accurately measure execution time of code sections.
 */
class PERF_prof_sect
{
public:
    /**
     * @brief Constructor for a profiling section
     *
     * Initializes all timing statistics to zero and sets up the section
     * identifier. All timeval structures are cleared to ensure clean
     * initial state.
     *
     * @param id String identifier for this profiling section (e.g., "combat_processing")
     */
    explicit PERF_prof_sect(const char *id)
        : mId( id )                    // Store the section identifier
        , mLastEnterTime( )            // Timestamp of most recent Enter() call
        , mPulseTotal( )               // Total time spent in this section this pulse
        , mPulseMax( )                 // Maximum single entry time this pulse
        , mTotal( )                    // Total time spent in this section ever
        , mMax( )                      // Maximum single entry time ever
        , mPulseEnterCount( 0 )        // Number of times entered this pulse
        , mPulseExitCount( 0 )         // Number of times exited this pulse
        , mTotalEnterCount( 0 )        // Number of times entered ever
    {
        // Initialize all timeval structures to zero
        timerclear(&mLastEnterTime);
        timerclear(&mPulseTotal);
        timerclear(&mPulseMax);
        timerclear(&mTotal);
        timerclear(&mMax);
    }

    /**
     * @brief Reset per-pulse statistics for a new game loop iteration
     */
    void PulseReset();

    /**
     * @brief Mark entry into this profiled code section
     */
    inline void Enter();

    /**
     * @brief Mark exit from this profiled code section
     */
    inline void Exit();

    // Accessor methods for retrieving statistics
    const std::string & GetId() const { return mId; }                                    ///< Get section identifier

    const struct timeval & GetPulseTotal() const { return mPulseTotal; }                 ///< Get total time this pulse
    const struct timeval & GetPulseMax() const { return mPulseMax; }                     ///< Get max single entry this pulse
    const struct timeval & GetTotal() const { return mTotal; }                           ///< Get total time ever
    const struct timeval & GetMax() const { return mMax; }                               ///< Get max single entry ever

    unsigned long int GetPulseEnterCount() const { return mPulseEnterCount; }            ///< Get enter count this pulse
    unsigned long int GetPulseExitCount() const { return mPulseExitCount; }              ///< Get exit count this pulse
    unsigned long int GetTotalEnterCount() const { return mTotalEnterCount; }            ///< Get enter count ever

private:
    // Prevent copying - these objects manage timing state and should not be copied
    PERF_prof_sect( const PERF_prof_sect & ); // unimplemented
    PERF_prof_sect & operator=( const PERF_prof_sect & ); // unimplemented

    std::string mId;                        ///< Unique identifier for this section
    struct timeval mLastEnterTime;          ///< Timestamp when Enter() was last called
    struct timeval mPulseTotal;             ///< Total time spent in section this pulse
    struct timeval mPulseMax;               ///< Maximum single entry time this pulse
    struct timeval mTotal;                  ///< Total time spent in section ever
    struct timeval mMax;                    ///< Maximum single entry time ever
    unsigned long int mPulseEnterCount;     ///< Number of Enter() calls this pulse
    unsigned long int mPulseExitCount;      ///< Number of Exit() calls this pulse
    unsigned long int mTotalEnterCount;     ///< Number of Enter() calls ever
};

/* ========================================================================
 * PROFILING MANAGER CLASS
 * ======================================================================== */

/**
 * @brief Manager class for all profiling sections
 *
 * This class maintains a registry of all profiling sections and provides
 * centralized management functions. It uses a map to store sections by
 * their string identifiers, allowing fast lookup and ensuring each
 * section is created only once.
 *
 * The manager provides functions to:
 * - Create new profiling sections on demand
 * - Reset all sections for a new pulse
 * - Generate various types of performance reports
 */
class PerfProfMgr
{
public:
    /**
     * @brief Constructor - initializes empty section registry
     */
    PerfProfMgr()
        : mSections( )  // Initialize empty map of profiling sections
    {
        // Constructor body is empty - all initialization done in member initializer list
    }

    /**
     * @brief Destructor - cleans up all allocated profiling sections
     */
    ~PerfProfMgr()
    {
        // Clean up all allocated profiling sections to prevent memory leaks
        for ( auto &&entry : mSections )
        {
            delete entry.second;
        }
        mSections.clear();
    }

    /**
     * @brief Create a new profiling section or return existing one
     * @param id String identifier for the section
     * @return Pointer to the profiling section
     */
    PERF_prof_sect *NewSection(const char *id);

    /**
     * @brief Reset per-pulse statistics for all sections
     */
    void ResetAll();

    /**
     * @brief Generate per-pulse profiling report for all sections
     * @param out_buf Buffer to store the report
     * @param n Size of the buffer
     * @return Number of characters written
     */
    size_t ReprPulse( char *out_buf, size_t n ) const { return ReprBase( out_buf, n, false, nullptr ); }

    /**
     * @brief Generate cumulative profiling report for all sections
     * @param out_buf Buffer to store the report
     * @param n Size of the buffer
     * @return Number of characters written
     */
    size_t ReprTotal( char *out_buf, size_t n ) const { return ReprBase( out_buf, n, true,  nullptr ); }

    /**
     * @brief Generate detailed report for a specific section
     * @param out_buf Buffer to store the report
     * @param n Size of the buffer
     * @param id Identifier of the section to report on
     * @return Number of characters written
     */
    size_t ReprSect( char *out_buf, size_t n, const char *id ) const;

private:
    // Prevent copying - these objects manage resources and should not be copied
    PerfProfMgr( const PerfProfMgr & ); // unimplemented
    PerfProfMgr & operator=( const PerfProfMgr & ); // unimplemented

    /**
     * @brief Internal function to generate formatted profiling reports
     * @param out_buf Buffer to store the report
     * @param n Size of the buffer
     * @param isTotal True for cumulative stats, false for per-pulse stats
     * @param sect Specific section to report on, or nullptr for all sections
     * @return Number of characters written
     */
    size_t ReprBase( char *out_buf, size_t n, bool isTotal, const PERF_prof_sect *sect ) const;

    /**
     * @brief Format output for a single profiling section
     * @param os Output stream to write to
     * @param sect Profiling section to output
     * @param isTotal True for cumulative stats, false for per-pulse stats
     */
    void SectOutput( std::ostream &os, const PERF_prof_sect *sect, bool isTotal) const;

    std::map<std::string, PERF_prof_sect *> mSections;  ///< Registry of all profiling sections
};

/**
 * @brief Generate a detailed report for a specific profiling section
 *
 * This method creates a comprehensive report for a single named profiling
 * section, showing both per-pulse and cumulative statistics. This is useful
 * for detailed analysis of a specific code section's performance.
 *
 * @param out_buf Buffer to store the formatted report
 * @param n Size of the output buffer
 * @param id String identifier of the section to report on
 * @return Number of characters written to the buffer
 */
size_t
PerfProfMgr::ReprSect(char *out_buf, size_t n, const char *id) const
{
    // Input validation
    if (!out_buf)
    {
        return 0;
    }
    if (n < 1)
    {
        out_buf[0] = '\0';
        return 0;
    }

    // Look up the requested section in our registry
    auto &&it = mSections.find(id);
    if (it == mSections.end())
    {
        // Section not found - generate error message
        std::ostringstream os;
        os << "No such section '" << id << "'\n\r";
        std::string str = os.str();

        // Ensure we don't exceed buffer bounds
        size_t max_copy = (n > 0) ? n - 1 : 0;  // Leave room for null terminator
        size_t to_copy = std::min(str.length(), max_copy);

        if (to_copy > 0)
        {
            str.copy( out_buf, to_copy );
        }

        out_buf[to_copy] = '\0';  // Ensure null termination
        return to_copy;
    }

    // Generate both per-pulse and cumulative reports for this section
    size_t copied = ReprBase(out_buf, n, false, it->second);  // Per-pulse report

    // Ensure we don't exceed buffer bounds for the second report
    if (copied < n)
    {
        copied += ReprBase(out_buf + copied, n - copied, true, it->second);  // Cumulative report
    }

    return copied;
}

/**
 * @brief Format and output statistics for a single profiling section
 *
 * This method generates a formatted table row showing performance statistics
 * for one profiling section. The output format depends on whether per-pulse
 * or cumulative statistics are requested.
 *
 * For per-pulse reports, shows:
 * - Section name, enter count, exit count, total microseconds, pulse %, max pulse %
 *
 * For cumulative reports, shows:
 * - Section name, enter count, total microseconds, total %, max pulse %
 *
 * @param os Output stream to write the formatted data to
 * @param sect Profiling section to output statistics for
 * @param isTotal True for cumulative stats, false for per-pulse stats
 */
void
PerfProfMgr::SectOutput(std::ostream &os, const PERF_prof_sect *sect, bool isTotal) const
{
    unsigned long int enterCount;
    long int usecTotal;
    long int usecMax;

    // Extract the appropriate statistics based on report type
    if (isTotal)
    {
        // Cumulative statistics since server startup
        enterCount = sect->GetTotalEnterCount();
        usecTotal = USEC_TOTAL( &(sect->GetTotal()) );
        usecMax = USEC_TOTAL( &(sect->GetMax()) );
    }
    else
    {
        // Per-pulse statistics for current pulse
        enterCount = sect->GetPulseEnterCount();
        usecTotal = USEC_TOTAL( &(sect->GetPulseTotal()) );
        usecMax = USEC_TOTAL( &(sect->GetPulseMax()) );
    }

    // Skip sections that weren't active (no point showing empty rows)
    if ( enterCount < 1 )
    {
        return;
    }

    // Format the table row with proper column alignment
    os << std::left
       << std::setw(20) << sect->GetId() << "|"     // Section name (left-aligned)
       << std::right
       << std::setw(12) << enterCount << "|";       // Enter count (right-aligned)

    // For per-pulse reports, also show exit count
    if (!isTotal)
    {
       os << std::setw(12) << sect->GetPulseExitCount() << "|";
    }

    os << std::setw(12) << usecTotal << "|";        // Total microseconds

    // Calculate and display percentage of time consumed
    if (isTotal)
    {
        // For cumulative reports: percentage of total server uptime
        time_t total_secs = time(NULL) - init_time;
        double sect_secs = static_cast<double>(usecTotal) / 1000000;

        os << std::setw(12-1) << ( 100 * sect_secs / total_secs) << "%|";
    }
    else
    {
        // For per-pulse reports: percentage of current pulse time
        os << std::setw(12-1) << ( 100 * static_cast<double>(usecTotal) / USEC_PER_PULSE ) << "%|";
    }

    // Maximum single entry time as percentage of pulse time
    os << std::setw(20-1) << ( 100 * static_cast<double>(usecMax) / USEC_PER_PULSE ) << "%\n\r";
}

/**
 * @brief Generate the base profiling report with formatted table
 *
 * This is the core report generation function that creates formatted tables
 * showing profiling statistics. It can generate either per-pulse or cumulative
 * reports, and can show either all sections or a specific section.
 *
 * The report format includes:
 * - Header indicating report type (pulse vs cumulative)
 * - Column headers with proper alignment
 * - Separator line
 * - Data rows for each active profiling section
 *
 * @param out_buf Buffer to store the formatted report
 * @param n Size of the output buffer
 * @param isTotal True for cumulative stats, false for per-pulse stats
 * @param sect Specific section to report on, or nullptr for all sections
 * @return Number of characters written to the buffer
 */
size_t
PerfProfMgr::ReprBase(char *out_buf, size_t n, bool isTotal, const PERF_prof_sect *sect) const
{
    // Input validation
    if (!out_buf)
    {
        return 0;
    }
    if (n < 1)
    {
        out_buf[0] = '\0';
        return 0;
    }

    std::ostringstream os;

    // Generate appropriate header based on report type
    if (isTotal)
    {
        os << "Cumulative profiling info\n\r";
    }
    else
    {
        os << "Pulse profiling info\n\r";
    }

    // Set up formatting for the table (2 decimal places, fixed-point notation)
    os << std::setprecision(2) << std::fixed
       << "\n\r"
       << std::left
       << std::setw(20) << "Section name" << "|"
       << std::right
       << std::setw(12) << "Enter Count" << "|";

    // Per-pulse reports include exit count column
    if (!isTotal)
    {
        os << std::setw(12) << "Exit Count" << "|";
    }

    os << std::setw(12) << "usec total" << "|";

    // Different percentage column headers for different report types
    if (isTotal)
    {
        os << std::setw(12) << "total %" << "|";        // % of total server uptime
    }
    else
    {
        os << std::setw(12) << "pulse %" << "|";        // % of current pulse time
    }
    os << std::setw(20) << "max pulse % (1 entry)" << "\n\r";

    // Add separator line
    os << std::setfill('-') << std::setw(80) << " " << std::setfill(' ') << "\n\r" ;

    // Output data rows
    if (sect)
    {
        // Single section report
        SectOutput(os, sect, isTotal);
    }
    else
    {
        // All sections report
        for ( auto &&entry : this->mSections )
        {
            SectOutput(os, entry.second, isTotal);
        }
    }

    // Convert to C-style string and return
    std::string str = os.str();

    // Ensure we don't exceed buffer bounds
    size_t max_copy = (n > 0) ? n - 1 : 0;  // Leave room for null terminator
    size_t to_copy = std::min(str.length(), max_copy);

    if (to_copy > 0)
    {
        str.copy( out_buf, to_copy );
    }

    out_buf[to_copy] = '\0';  // Ensure null termination

    return to_copy;
}

/**
 * @brief Create a new profiling section or return existing one
 *
 * This method implements lazy initialization of profiling sections. If a
 * section with the given ID already exists, it returns the existing one.
 * Otherwise, it creates a new section, adds it to the registry, and returns it.
 *
 * This implementation properly checks for existing sections to prevent
 * memory leaks and ensures each section ID is unique.
 *
 * @param id String identifier for the profiling section
 * @return Pointer to the profiling section (new or existing)
 */
PERF_prof_sect *
PerfProfMgr::NewSection(const char *id)
{
    // Input validation
    if (!id)
    {
        return nullptr;
    }

    // Check if section already exists
    auto it = mSections.find(id);
    if (it != mSections.end())
    {
        // Return existing section
        return it->second;
    }

    // Create new section and add to registry
    PERF_prof_sect *ptr = new PERF_prof_sect( id );
    mSections[id] = ptr;
    return ptr;
}

/**
 * @brief Reset per-pulse statistics for all profiling sections
 *
 * This method is called at the beginning of each game loop iteration to
 * clear the per-pulse timing statistics for all profiling sections.
 * This ensures that each pulse starts with clean timing data.
 *
 * Cumulative statistics are preserved - only per-pulse data is reset.
 */
void
PerfProfMgr::ResetAll()
{
    // Iterate through all registered sections and reset their pulse data
    for ( auto &&entry : this->mSections )
    {
        entry.second->PulseReset();
    }
}

/* ========================================================================
 * PROFILING SECTION METHODS
 * ======================================================================== */

/**
 * @brief Reset per-pulse statistics for this section
 *
 * This method clears all per-pulse timing data, preparing the section for
 * a new game loop iteration. It resets counters and timing accumulators
 * but preserves cumulative statistics.
 *
 * Called automatically by PerfProfMgr::ResetAll() at the start of each pulse.
 */
void
PERF_prof_sect::PulseReset()
{
    mPulseEnterCount = 0;           // Reset enter count for this pulse
    mPulseExitCount = 0;            // Reset exit count for this pulse
    timerclear( &mPulseTotal );     // Reset total time for this pulse
    timerclear( &mPulseMax );       // Reset max single entry time for this pulse
}

/**
 * @brief Mark entry into this profiled code section
 *
 * Records the current timestamp and increments entry counters. This timestamp
 * will be used by Exit() to calculate the time spent in this section.
 *
 * Must be paired with a corresponding Exit() call for accurate timing.
 */
void
PERF_prof_sect::Enter()
{
    ++mPulseEnterCount;                     // Increment per-pulse enter counter
    ++mTotalEnterCount;                     // Increment cumulative enter counter
    gettimeofday(&mLastEnterTime, NULL);    // Record entry timestamp
}

/**
 * @brief Mark exit from this profiled code section
 *
 * Calculates the elapsed time since the corresponding Enter() call and
 * updates both per-pulse and cumulative timing statistics. Also tracks
 * the maximum single entry time for performance spike detection.
 */
void
PERF_prof_sect::Exit()
{
    ++mPulseExitCount;                      // Increment per-pulse exit counter
    struct timeval now;                     // Current timestamp
    struct timeval diff;                    // Time difference (elapsed time)
    gettimeofday(&now, NULL);               // Get current time

    // Calculate elapsed time since Enter() was called
    timersub(&now, &mLastEnterTime, &diff);

    // Add elapsed time to per-pulse and cumulative totals
    timeradd(&mPulseTotal, &diff, &mPulseTotal);
    timeradd(&mTotal, &diff, &mTotal);

    // Update maximum single entry time for this pulse if this was longer
    if ( timercmp(&diff, &mPulseMax, > ) )
    {
        mPulseMax = diff;
    }

    // Update maximum single entry time ever if this was longer
    if ( timercmp(&diff, &mMax, > ) )
    {
        mMax = diff;
    }
}

/* ========================================================================
 * GLOBAL PROFILING MANAGER INSTANCE
 * ======================================================================== */

/**
 * @brief Global instance of the profiling manager
 *
 * This static instance manages all profiling sections for the entire
 * application. It's created once at program startup and persists for
 * the lifetime of the application.
 */
static PerfProfMgr sProfMgr;

/* ========================================================================
 * PUBLIC API FUNCTIONS - CODE PROFILING
 * ======================================================================== */

/**
 * @brief Initialize a profiling section (C API wrapper)
 *
 * This function implements lazy initialization for profiling sections.
 * If the section pointer is already initialized (non-NULL), it does nothing.
 * Otherwise, it creates a new section through the global manager.
 *
 * This function is typically called automatically by the PERF_PROF_ENTER macro.
 *
 * @param ptr Pointer to section pointer (will be set if currently NULL)
 * @param id String identifier for the profiling section
 */
void PERF_prof_sect_init(PERF_prof_sect **ptr, const char *id)
{
    // Input validation
    if (!ptr || !id)
    {
        return;
    }

    // If already initialized, do nothing (lazy initialization)
    if (*ptr)
    {
        return;
    }

    // Create new section through the global manager
    *ptr = sProfMgr.NewSection(id);
}

/**
 * @brief Enter a profiling section (C API wrapper)
 *
 * Simple wrapper around the PERF_prof_sect::Enter() method.
 * Records entry timestamp and increments counters.
 *
 * @param ptr Pointer to the profiling section
 */
void PERF_prof_sect_enter(PERF_prof_sect *ptr)
{
    // Input validation
    if (!ptr)
    {
        return;
    }

    ptr->Enter();
}

/**
 * @brief Exit a profiling section (C API wrapper)
 *
 * Simple wrapper around the PERF_prof_sect::Exit() method.
 * Calculates elapsed time and updates statistics.
 *
 * @param ptr Pointer to the profiling section
 */
void PERF_prof_sect_exit(PERF_prof_sect *ptr)
{
    // Input validation
    if (!ptr)
    {
        return;
    }

    ptr->Exit();
}

/**
 * @brief Reset all profiling sections for a new pulse (C API wrapper)
 *
 * Wrapper around PerfProfMgr::ResetAll(). This should be called at the
 * beginning of each game loop iteration to clear per-pulse statistics.
 */
void PERF_prof_reset( void )
{
    sProfMgr.ResetAll();
}

/**
 * @brief Generate per-pulse profiling report (C API wrapper)
 *
 * @param out_buf Buffer to store the report
 * @param n Size of the buffer
 * @return Number of characters written
 */
size_t PERF_prof_repr_pulse( char *out_buf, size_t n )
{
    return sProfMgr.ReprPulse(out_buf, n);
}

/**
 * @brief Generate cumulative profiling report (C API wrapper)
 *
 * @param out_buf Buffer to store the report
 * @param n Size of the buffer
 * @return Number of characters written
 */
size_t PERF_prof_repr_total( char *out_buf, size_t n )
{
    return sProfMgr.ReprTotal(out_buf, n);
}

/**
 * @brief Generate report for specific profiling section (C API wrapper)
 *
 * @param out_buf Buffer to store the report
 * @param n Size of the buffer
 * @param id String identifier of the section to report on
 * @return Number of characters written
 */
size_t PERF_prof_repr_sect( char *out_buf, size_t n, const char *id)
{
    return sProfMgr.ReprSect(out_buf, n, id);
}

/* Fallback implementation if perfmon_optimized.c is not linked */
__attribute__((weak))
void PERF_log_pulse_optimized(double val)
{
    /* Default to full monitoring if optimized version not available */
    PERF_log_pulse(val);
}
