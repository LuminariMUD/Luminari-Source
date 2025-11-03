-- ============================================================================
-- LuminariMUD PubSub V3 Database Schema
-- Phase 3.1.4: Enhanced Message Storage and Metadata
-- Created: August 13, 2025
-- ============================================================================

-- Enhanced Messages Table (alongside existing pubsub_messages)
CREATE TABLE IF NOT EXISTS pubsub_messages_v3 (
    message_id INT PRIMARY KEY AUTO_INCREMENT,
    topic_id INT NOT NULL,
    sender_name VARCHAR(80) NOT NULL,
    sender_id BIGINT,
    message_type INT NOT NULL DEFAULT 0,
    message_category INT NOT NULL DEFAULT 1,
    priority INT NOT NULL DEFAULT 2,
    content TEXT,
    content_type VARCHAR(64) DEFAULT 'text/plain',
    content_encoding VARCHAR(32) DEFAULT 'utf-8',
    content_version INT DEFAULT 1,
    spatial_data TEXT,
    legacy_metadata TEXT,              -- For backward compatibility
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NULL,
    last_modified_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    delivery_attempts INT DEFAULT 0,
    successful_deliveries INT DEFAULT 0,
    failed_deliveries INT DEFAULT 0,
    is_processed BOOLEAN DEFAULT FALSE,
    processed_at TIMESTAMP NULL,
    reference_count INT DEFAULT 1,
    
    -- V3 Enhanced Fields
    parent_message_id INT NULL,        -- For message threading
    thread_id INT NULL,                -- Conversation threading
    sequence_number INT DEFAULT 0,     -- Order in thread
    
    -- Routing and routing keys
    routing_keys TEXT,                 -- JSON array of routing keys
    
    -- Indexes for performance
    INDEX idx_topic_id (topic_id),
    INDEX idx_sender_id (sender_id),
    INDEX idx_message_type (message_type),
    INDEX idx_message_category (message_category),
    INDEX idx_created_at (created_at),
    INDEX idx_expires_at (expires_at),
    INDEX idx_parent_message (parent_message_id),
    INDEX idx_thread_id (thread_id),
    INDEX idx_is_processed (is_processed),
    INDEX idx_composite_type_cat (message_type, message_category),
    
    -- Foreign key constraints (if topics table exists)
    -- FOREIGN KEY (topic_id) REFERENCES pubsub_topics(topic_id) ON DELETE CASCADE,
    FOREIGN KEY (parent_message_id) REFERENCES pubsub_messages_v3(message_id) ON DELETE SET NULL
);

-- Enhanced Metadata Table
CREATE TABLE IF NOT EXISTS pubsub_message_metadata_v3 (
    metadata_id INT PRIMARY KEY AUTO_INCREMENT,
    message_id INT NOT NULL,
    
    -- Sender Information
    sender_real_name VARCHAR(80),
    sender_title VARCHAR(120),
    sender_level INT,
    sender_class VARCHAR(40),
    sender_race VARCHAR(40),
    
    -- Origin Information  
    origin_room INT,
    origin_zone INT,
    origin_area_name VARCHAR(80),
    origin_x INT,
    origin_y INT,
    origin_z INT,
    
    -- Contextual Information
    context_type VARCHAR(40),          -- EVENT/COMMAND/SYSTEM/TRIGGER
    trigger_event VARCHAR(80),
    related_object VARCHAR(80),
    related_object_id INT,
    
    -- Processing Information
    handler_chain TEXT,                -- JSON array of handlers
    processing_time_ms INT DEFAULT 0,
    processing_notes TEXT,
    
    -- Metadata timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    -- Indexes
    INDEX idx_message_id (message_id),
    INDEX idx_sender_level (sender_level),
    INDEX idx_origin_room (origin_room),
    INDEX idx_origin_zone (origin_zone),
    INDEX idx_context_type (context_type),
    INDEX idx_related_object_id (related_object_id),
    
    -- Foreign key
    FOREIGN KEY (message_id) REFERENCES pubsub_messages_v3(message_id) ON DELETE CASCADE
);

-- Custom Fields Table for Extensible Message Data
CREATE TABLE IF NOT EXISTS pubsub_message_fields_v3 (
    field_id INT PRIMARY KEY AUTO_INCREMENT,
    message_id INT NOT NULL,
    field_name VARCHAR(64) NOT NULL,
    field_type INT NOT NULL,           -- PUBSUB_FIELD_TYPE_* constants
    
    -- Value storage (use appropriate field based on type)
    string_value TEXT,
    integer_value BIGINT,
    float_value DOUBLE,
    boolean_value BOOLEAN,
    timestamp_value TIMESTAMP NULL,
    json_value JSON,                   -- For complex data structures
    
    -- Additional metadata for fields
    field_description VARCHAR(255),
    is_searchable BOOLEAN DEFAULT FALSE,
    is_indexed BOOLEAN DEFAULT FALSE,
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    -- Indexes
    INDEX idx_message_id (message_id),
    INDEX idx_field_name (field_name),
    INDEX idx_field_type (field_type),
    INDEX idx_string_value (string_value(191)),  -- MySQL index length limit
    INDEX idx_integer_value (integer_value),
    INDEX idx_float_value (float_value),
    INDEX idx_boolean_value (boolean_value),
    INDEX idx_timestamp_value (timestamp_value),
    INDEX idx_is_searchable (is_searchable),
    
    -- Foreign key
    FOREIGN KEY (message_id) REFERENCES pubsub_messages_v3(message_id) ON DELETE CASCADE,
    
    -- Unique constraint for field names per message
    UNIQUE KEY unique_message_field (message_id, field_name)
);

-- Message Tags Table for Classification and Filtering
CREATE TABLE IF NOT EXISTS pubsub_message_tags_v3 (
    tag_id INT PRIMARY KEY AUTO_INCREMENT,
    message_id INT NOT NULL,
    tag_category VARCHAR(40) NOT NULL, -- location, player, event, etc.
    tag_name VARCHAR(80) NOT NULL,
    tag_value VARCHAR(255),            -- Optional value for parameterized tags
    
    -- Tag metadata
    is_system_tag BOOLEAN DEFAULT FALSE,  -- System vs user-defined
    weight INT DEFAULT 1,                 -- Tag importance/weight
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    -- Indexes
    INDEX idx_message_id (message_id),
    INDEX idx_tag_category (tag_category),
    INDEX idx_tag_name (tag_name),
    INDEX idx_tag_value (tag_value(191)),
    INDEX idx_is_system_tag (is_system_tag),
    INDEX idx_composite_cat_name (tag_category, tag_name),
    
    -- Foreign key
    FOREIGN KEY (message_id) REFERENCES pubsub_messages_v3(message_id) ON DELETE CASCADE
);

-- Message Performance and Statistics Table
CREATE TABLE IF NOT EXISTS pubsub_message_stats_v3 (
    stat_id INT PRIMARY KEY AUTO_INCREMENT,
    message_id INT NOT NULL,
    
    -- Performance metrics
    creation_time_ms INT DEFAULT 0,
    total_processing_time_ms INT DEFAULT 0,
    delivery_time_ms INT DEFAULT 0,
    
    -- Usage statistics
    view_count INT DEFAULT 0,
    response_count INT DEFAULT 0,       -- Number of replies/responses
    forward_count INT DEFAULT 0,        -- Number of times forwarded
    
    -- Quality metrics
    spam_score FLOAT DEFAULT 0.0,       -- Spam detection score
    sentiment_score FLOAT DEFAULT 0.0,  -- Message sentiment (-1 to 1)
    relevance_score FLOAT DEFAULT 0.0,  -- Message relevance
    
    -- Tracking
    last_accessed TIMESTAMP NULL,
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    -- Indexes
    INDEX idx_message_id (message_id),
    INDEX idx_creation_time (creation_time_ms),
    INDEX idx_view_count (view_count),
    INDEX idx_spam_score (spam_score),
    INDEX idx_last_accessed (last_accessed),
    
    -- Foreign key
    FOREIGN KEY (message_id) REFERENCES pubsub_messages_v3(message_id) ON DELETE CASCADE
);

-- Message Filters and Subscriptions for V3
CREATE TABLE IF NOT EXISTS pubsub_filters_v3 (
    filter_id INT PRIMARY KEY AUTO_INCREMENT,
    subscriber_id BIGINT NOT NULL,     -- Character or system ID
    filter_name VARCHAR(80) NOT NULL,
    
    -- Filter criteria
    topic_ids TEXT,                    -- JSON array of topic IDs
    message_types TEXT,                -- JSON array of message types
    message_categories TEXT,           -- JSON array of categories
    sender_patterns TEXT,              -- JSON array of sender name patterns
    content_patterns TEXT,             -- JSON array of content patterns
    tag_filters TEXT,                  -- JSON object with tag criteria
    
    -- Filter options
    priority_min INT DEFAULT 0,
    priority_max INT DEFAULT 10,
    include_spatial BOOLEAN DEFAULT TRUE,
    include_legacy BOOLEAN DEFAULT TRUE,
    max_age_hours INT DEFAULT 24,
    
    -- Filter metadata
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_used TIMESTAMP NULL,
    use_count INT DEFAULT 0,
    
    -- Indexes
    INDEX idx_subscriber_id (subscriber_id),
    INDEX idx_filter_name (filter_name),
    INDEX idx_is_active (is_active),
    INDEX idx_last_used (last_used),
    
    -- Unique constraint
    UNIQUE KEY unique_subscriber_filter (subscriber_id, filter_name)
);

-- ============================================================================
-- Views for Common Queries
-- ============================================================================

-- View combining message and metadata for easy querying
CREATE OR REPLACE VIEW pubsub_messages_full_v3 AS
SELECT 
    m.*,
    md.sender_real_name,
    md.sender_title,
    md.sender_level,
    md.sender_class,
    md.sender_race,
    md.origin_room,
    md.origin_zone,
    md.origin_area_name,
    md.context_type,
    md.trigger_event,
    md.processing_time_ms,
    s.view_count,
    s.response_count,
    s.spam_score,
    s.sentiment_score
FROM pubsub_messages_v3 m
LEFT JOIN pubsub_message_metadata_v3 md ON m.message_id = md.message_id
LEFT JOIN pubsub_message_stats_v3 s ON m.message_id = s.message_id;

-- View for message threads
CREATE OR REPLACE VIEW pubsub_message_threads_v3 AS
SELECT 
    thread_id,
    COUNT(*) as message_count,
    MIN(created_at) as thread_started,
    MAX(created_at) as last_activity,
    GROUP_CONCAT(DISTINCT sender_name ORDER BY created_at) as participants
FROM pubsub_messages_v3 
WHERE thread_id IS NOT NULL
GROUP BY thread_id;

-- ============================================================================
-- Stored Procedures for Common Operations
-- ============================================================================

DELIMITER //

-- Procedure to create a complete V3 message with metadata
CREATE PROCEDURE CreateMessageV3(
    IN p_topic_id INT,
    IN p_sender_name VARCHAR(80),
    IN p_sender_id BIGINT,
    IN p_message_type INT,
    IN p_message_category INT,
    IN p_priority INT,
    IN p_content TEXT,
    IN p_content_type VARCHAR(64),
    IN p_sender_real_name VARCHAR(80),
    IN p_sender_level INT,
    IN p_origin_room INT,
    IN p_origin_zone INT,
    OUT p_message_id INT
)
BEGIN
    DECLARE EXIT HANDLER FOR SQLEXCEPTION
    BEGIN
        ROLLBACK;
        RESIGNAL;
    END;
    
    START TRANSACTION;
    
    -- Insert main message
    INSERT INTO pubsub_messages_v3 (
        topic_id, sender_name, sender_id, message_type, 
        message_category, priority, content, content_type
    ) VALUES (
        p_topic_id, p_sender_name, p_sender_id, p_message_type,
        p_message_category, p_priority, p_content, p_content_type
    );
    
    SET p_message_id = LAST_INSERT_ID();
    
    -- Insert metadata if provided
    IF p_sender_real_name IS NOT NULL OR p_sender_level IS NOT NULL 
       OR p_origin_room IS NOT NULL OR p_origin_zone IS NOT NULL THEN
        INSERT INTO pubsub_message_metadata_v3 (
            message_id, sender_real_name, sender_level, origin_room, origin_zone
        ) VALUES (
            p_message_id, p_sender_real_name, p_sender_level, p_origin_room, p_origin_zone
        );
    END IF;
    
    -- Initialize statistics
    INSERT INTO pubsub_message_stats_v3 (message_id) VALUES (p_message_id);
    
    COMMIT;
END //

-- Procedure to add a tag to a message
CREATE PROCEDURE AddMessageTag(
    IN p_message_id INT,
    IN p_tag_category VARCHAR(40),
    IN p_tag_name VARCHAR(80),
    IN p_tag_value VARCHAR(255),
    IN p_is_system_tag BOOLEAN
)
BEGIN
    INSERT INTO pubsub_message_tags_v3 (
        message_id, tag_category, tag_name, tag_value, is_system_tag
    ) VALUES (
        p_message_id, p_tag_category, p_tag_name, p_tag_value, p_is_system_tag
    ) ON DUPLICATE KEY UPDATE
        tag_value = VALUES(tag_value),
        is_system_tag = VALUES(is_system_tag);
END //

DELIMITER ;

-- ============================================================================
-- Initial Data and Configuration
-- ============================================================================

-- Insert default message type mappings (for reference)
INSERT IGNORE INTO pubsub_message_fields_v3 (message_id, field_name, field_type, string_value, field_description) VALUES
(0, '_message_types', 1, '{"0":"text","1":"formatted","2":"spatial","3":"system","4":"personal","5":"event","6":"quest","7":"combat","8":"weather","9":"economy","10":"social","11":"administrative","12":"debug"}', 'Message type reference'),
(0, '_message_categories', 1, '{"1":"communication","2":"environment","3":"gameplay","4":"social","5":"system","6":"economy","7":"exploration","8":"roleplay","9":"pvp","10":"custom"}', 'Message category reference');

-- ============================================================================
-- Migration and Maintenance Procedures
-- ============================================================================

DELIMITER //

-- Procedure to migrate legacy messages to V3 format
CREATE PROCEDURE MigrateLegacyToV3()
BEGIN
    DECLARE done INT DEFAULT FALSE;
    DECLARE v_message_id, v_topic_id, v_sender_id, v_message_type, v_priority INT;
    DECLARE v_sender_name, v_content, v_spatial_data, v_metadata VARCHAR(1000);
    DECLARE v_created_at, v_expires_at TIMESTAMP;
    
    DECLARE legacy_cursor CURSOR FOR 
        SELECT message_id, topic_id, sender_name, sender_id, message_type, 
               priority, content, spatial_data, metadata, created_at, expires_at
        FROM pubsub_messages 
        WHERE message_id NOT IN (SELECT message_id FROM pubsub_messages_v3);
    
    DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;
    
    OPEN legacy_cursor;
    
    migration_loop: LOOP
        FETCH legacy_cursor INTO v_message_id, v_topic_id, v_sender_name, v_sender_id,
              v_message_type, v_priority, v_content, v_spatial_data, v_metadata,
              v_created_at, v_expires_at;
        
        IF done THEN
            LEAVE migration_loop;
        END IF;
        
        -- Insert into V3 table with legacy data
        INSERT INTO pubsub_messages_v3 (
            message_id, topic_id, sender_name, sender_id, message_type,
            message_category, priority, content, spatial_data, legacy_metadata,
            created_at, expires_at
        ) VALUES (
            v_message_id, v_topic_id, v_sender_name, v_sender_id, v_message_type,
            1, v_priority, v_content, v_spatial_data, v_metadata,
            v_created_at, v_expires_at
        );
        
        -- Initialize statistics for migrated message
        INSERT INTO pubsub_message_stats_v3 (message_id) VALUES (v_message_id);
        
    END LOOP;
    
    CLOSE legacy_cursor;
END //

DELIMITER ;

-- ============================================================================
-- Comments and Documentation
-- ============================================================================

/*
PubSub V3 Database Schema Notes:

1. Backward Compatibility:
   - Maintains compatibility with existing pubsub_messages table
   - Uses different table names (with _v3 suffix) to avoid conflicts
   - Includes legacy_metadata field for preserving old metadata

2. Performance Considerations:
   - Extensive indexing for common query patterns
   - Separate tables for metadata and fields to normalize data
   - JSON fields for flexible data storage where appropriate

3. Extensibility:
   - Custom fields table allows arbitrary message extensions
   - Tag system provides flexible classification
   - Statistics table enables performance monitoring

4. Data Integrity:
   - Foreign key constraints ensure referential integrity
   - Unique constraints prevent duplicate data
   - Proper timestamp handling with automatic updates

5. Migration Path:
   - MigrateLegacyToV3() procedure for upgrading existing data
   - Views provide compatibility layer for existing queries
   - Separate V3 tables allow gradual migration

6. Usage Examples:
   - Use CreateMessageV3() procedure for new messages
   - Query pubsub_messages_full_v3 view for complete message data
   - Use pubsub_message_threads_v3 view for conversation analysis
   - Filter messages using pubsub_filters_v3 table

7. Maintenance:
   - Regular cleanup of expired messages
   - Statistics aggregation for performance monitoring
   - Index optimization based on usage patterns
*/
