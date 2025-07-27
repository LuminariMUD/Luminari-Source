<?php
/**
 * config.php - Shared Configuration and Utilities for LuminariMUD PHP Tools
 * 
 * This file provides common functionality used across multiple PHP tools:
 * - Database connection management
 * - Security functions
 * - Input validation
 * - Error handling
 * - Authentication helpers
 * 
 * @see ../documentation/PHP_TOOLS_README.md for comprehensive security audit,
 *      deployment guide, and security best practices for all PHP tools.
 * 
 * @author LuminariMUD Development Team
 * @version 1.0
 * @since 2025-01-24
 */

// Prevent direct access
if (!defined('LUMINARI_TOOLS')) {
    http_response_code(403);
    die('Direct access not allowed');
}

/**
 * Security Configuration
 */
class SecurityConfig {
    
    /**
     * Initialize security settings
     */
    public static function init() {
        // Security headers
        header('X-Content-Type-Options: nosniff');
        header('X-Frame-Options: DENY');
        header('X-XSS-Protection: 1; mode=block');
        header('Referrer-Policy: strict-origin-when-cross-origin');
        
        // Start session with secure settings
        if (session_status() === PHP_SESSION_NONE) {
            ini_set('session.cookie_httponly', 1);
            ini_set('session.cookie_secure', 1);
            ini_set('session.use_strict_mode', 1);
            session_start();
        }
        
        // Generate CSRF token if not exists
        if (!isset($_SESSION['csrf_token'])) {
            $_SESSION['csrf_token'] = bin2hex(random_bytes(32));
        }
    }
    
    /**
     * Check if user is authenticated
     * 
     * @param array $required_roles Required roles for access
     * @return bool True if authenticated with required role
     */
    public static function isAuthenticated($required_roles = []) {
        if (!isset($_SESSION['authenticated']) || $_SESSION['authenticated'] !== true) {
            return false;
        }
        
        if (!empty($required_roles)) {
            $user_role = $_SESSION['role'] ?? '';
            return in_array($user_role, $required_roles, true);
        }
        
        return true;
    }
    
    /**
     * Validate CSRF token
     * 
     * @throws Exception If CSRF validation fails
     */
    public static function validateCSRF() {
        if (!isset($_POST['csrf_token']) || !isset($_SESSION['csrf_token']) ||
            !hash_equals($_SESSION['csrf_token'], $_POST['csrf_token'])) {
            error_log("CSRF token validation failed from IP: " . ($_SERVER['REMOTE_ADDR'] ?? 'unknown'));
            http_response_code(403);
            throw new Exception("CSRF token validation failed. Please refresh and try again.");
        }
    }
}

/**
 * Database Connection Manager
 */
class DatabaseManager {
    
    private static $pdo = null;
    
    /**
     * Get PDO database connection
     * 
     * @return PDO Database connection
     * @throws Exception If connection fails
     */
    public static function getConnection() {
        if (self::$pdo === null) {
            self::connect();
        }
        return self::$pdo;
    }
    
    /**
     * Establish database connection
     * 
     * @throws Exception If connection fails
     */
    private static function connect() {
        // Get credentials from environment variables
        $host = $_ENV['DB_HOST'] ?? getenv('DB_HOST') ?? 'localhost';
        $user = $_ENV['DB_USER'] ?? getenv('DB_USER') ?? '';
        $pass = $_ENV['DB_PASS'] ?? getenv('DB_PASS') ?? '';
        $name = $_ENV['DB_NAME'] ?? getenv('DB_NAME') ?? '';
        
        // Validate credentials are provided
        if (empty($host) || empty($user) || empty($pass) || empty($name)) {
            error_log("Database credentials not properly configured");
            throw new Exception("Database configuration error. Please contact administrator.");
        }
        
        try {
            self::$pdo = new PDO(
                "mysql:host=$host;dbname=$name;charset=utf8mb4",
                $user,
                $pass,
                [
                    PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
                    PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
                    PDO::ATTR_EMULATE_PREPARES => false,
                    PDO::MYSQL_ATTR_FOUND_ROWS => true
                ]
            );
        } catch (PDOException $e) {
            error_log("Database connection failed: " . $e->getMessage());
            throw new Exception("Database connection error. Please contact administrator.");
        }
    }
}

/**
 * Input Validation Utilities
 */
class InputValidator {
    
    /**
     * Validate and sanitize input
     * 
     * @param string $input The input to validate
     * @param string $type The type of input (identifier, text, number, email)
     * @param int $max_length Maximum allowed length
     * @return string|int|false Sanitized input or false if invalid
     */
    public static function validate($input, $type, $max_length = 255) {
        if (strlen($input) > $max_length) {
            return false;
        }
        
        switch ($type) {
            case 'identifier':
                // Only allow alphanumeric and underscores for C identifiers
                if (!preg_match('/^[a-zA-Z_][a-zA-Z0-9_]*$/', $input)) {
                    return false;
                }
                return trim($input);
                
            case 'text':
                // Remove potentially dangerous characters
                return preg_replace('/[<>"\'\\\]/', '', trim($input));
                
            case 'number':
                if (!is_numeric($input) || $input < 0 || $input > 999999) {
                    return false;
                }
                return (int)$input;
                
            case 'email':
                return filter_var($input, FILTER_VALIDATE_EMAIL);
                
            default:
                return false;
        }
    }
    
    /**
     * Validate input against whitelist
     * 
     * @param string $input Input to validate
     * @param array $allowed_values Whitelist of allowed values
     * @return string|false Valid input or false if not in whitelist
     */
    public static function validateWhitelist($input, $allowed_values) {
        return in_array($input, $allowed_values, true) ? $input : false;
    }
}

/**
 * Error Handling Utilities
 */
class ErrorHandler {
    
    /**
     * Handle validation error
     * 
     * @param string $message Error message
     * @param int $code HTTP status code
     */
    public static function validationError($message, $code = 400) {
        error_log("Validation error: $message from IP: " . ($_SERVER['REMOTE_ADDR'] ?? 'unknown'));
        http_response_code($code);
        die("Error: $message");
    }
    
    /**
     * Handle database error
     * 
     * @param Exception $e Database exception
     */
    public static function databaseError($e) {
        error_log("Database error: " . $e->getMessage());
        http_response_code(500);
        die("Database error. Please contact administrator.");
    }
    
    /**
     * Handle authentication error
     * 
     * @param string $tool_name Name of the tool being accessed
     */
    public static function authenticationError($tool_name) {
        error_log("Unauthorized access attempt to $tool_name from IP: " . ($_SERVER['REMOTE_ADDR'] ?? 'unknown'));
        http_response_code(403);
        die("Access denied. Authentication required for this tool.");
    }
}

/**
 * Caching Utilities
 */
class CacheManager {

    private static $cache_dir = 'cache/';
    private static $default_ttl = 3600; // 1 hour

    /**
     * Initialize cache directory
     */
    public static function init() {
        if (!is_dir(self::$cache_dir)) {
            mkdir(self::$cache_dir, 0755, true);
        }
    }

    /**
     * Get cached data
     *
     * @param string $key Cache key
     * @return mixed|false Cached data or false if not found/expired
     */
    public static function get($key) {
        self::init();
        $file = self::$cache_dir . md5($key) . '.cache';

        if (!file_exists($file)) {
            return false;
        }

        $data = unserialize(file_get_contents($file));
        if ($data['expires'] < time()) {
            unlink($file);
            return false;
        }

        return $data['content'];
    }

    /**
     * Set cached data
     *
     * @param string $key Cache key
     * @param mixed $data Data to cache
     * @param int $ttl Time to live in seconds
     */
    public static function set($key, $data, $ttl = null) {
        self::init();
        $ttl = $ttl ?? self::$default_ttl;
        $file = self::$cache_dir . md5($key) . '.cache';

        $cache_data = [
            'expires' => time() + $ttl,
            'content' => $data
        ];

        file_put_contents($file, serialize($cache_data), LOCK_EX);
    }

    /**
     * Clear cache by key or all cache
     *
     * @param string|null $key Specific key to clear, or null for all
     */
    public static function clear($key = null) {
        self::init();

        if ($key === null) {
            // Clear all cache
            $files = glob(self::$cache_dir . '*.cache');
            foreach ($files as $file) {
                unlink($file);
            }
        } else {
            // Clear specific key
            $file = self::$cache_dir . md5($key) . '.cache';
            if (file_exists($file)) {
                unlink($file);
            }
        }
    }
}

/**
 * HTML Utilities
 */
class HTMLHelper {
    
    /**
     * Generate CSRF token input field
     * 
     * @return string HTML input field
     */
    public static function csrfTokenField() {
        $token = htmlspecialchars($_SESSION['csrf_token'], ENT_QUOTES | ENT_HTML5, 'UTF-8');
        return "<input type=\"hidden\" name=\"csrf_token\" value=\"$token\">";
    }
    
    /**
     * Escape HTML output safely
     * 
     * @param string $text Text to escape
     * @return string Escaped text
     */
    public static function escape($text) {
        return htmlspecialchars($text, ENT_QUOTES | ENT_HTML5, 'UTF-8');
    }
    
    /**
     * Generate standard HTML document header
     * 
     * @param string $title Page title
     * @return string HTML header
     */
    public static function getHeader($title) {
        $escaped_title = self::escape($title);
        return "<!DOCTYPE html>
<html lang=\"en\">
<head>
    <meta charset=\"UTF-8\">
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">
    <meta name=\"robots\" content=\"noindex, nofollow\">
    <title>$escaped_title</title>";
    }
}

// Initialize security settings when this file is included
SecurityConfig::init();
