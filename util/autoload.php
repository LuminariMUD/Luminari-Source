<?php
/**
 * autoload.php - PSR-4 Autoloader for LuminariMUD PHP Tools
 * 
 * This file provides PSR-4 compliant autoloading for the LuminariMUD tools.
 * It follows modern PHP standards and best practices.
 * 
 * @see ../documentation/PHP_TOOLS_README.md for comprehensive security audit,
 *      deployment guide, and security best practices for all PHP tools.
 * 
 * @author LuminariMUD Development Team
 * @version 1.0
 * @since 2025-01-24
 */

/**
 * PSR-4 Autoloader Implementation
 * 
 * This autoloader follows PSR-4 standards for class loading.
 * Classes should be organized in the following structure:
 * 
 * LuminariMUD\
 *   Tools\
 *     Security\
 *     Database\
 *     Validation\
 *     Cache\
 *     HTML\
 */
spl_autoload_register(function (string $className): void {
    // Project-specific namespace prefix
    $prefix = 'LuminariMUD\\Tools\\';
    
    // Base directory for the namespace prefix
    $baseDir = __DIR__ . '/src/';
    
    // Does the class use the namespace prefix?
    $len = strlen($prefix);
    if (strncmp($prefix, $className, $len) !== 0) {
        // No, move to the next registered autoloader
        return;
    }
    
    // Get the relative class name
    $relativeClass = substr($className, $len);
    
    // Replace the namespace prefix with the base directory, replace namespace
    // separators with directory separators in the relative class name, append
    // with .php
    $file = $baseDir . str_replace('\\', '/', $relativeClass) . '.php';
    
    // If the file exists, require it
    if (file_exists($file)) {
        require $file;
    }
});

/**
 * Legacy class aliases for backward compatibility
 * 
 * These aliases allow existing code to continue working while
 * we transition to the new namespaced structure.
 */
if (!class_exists('SecurityConfig')) {
    class_alias('LuminariMUD\\Tools\\Security\\Config', 'SecurityConfig');
}

if (!class_exists('DatabaseManager')) {
    class_alias('LuminariMUD\\Tools\\Database\\Manager', 'DatabaseManager');
}

if (!class_exists('InputValidator')) {
    class_alias('LuminariMUD\\Tools\\Validation\\InputValidator', 'InputValidator');
}

if (!class_exists('ErrorHandler')) {
    class_alias('LuminariMUD\\Tools\\Error\\Handler', 'ErrorHandler');
}

if (!class_exists('HTMLHelper')) {
    class_alias('LuminariMUD\\Tools\\HTML\\Helper', 'HTMLHelper');
}

if (!class_exists('CacheManager')) {
    class_alias('LuminariMUD\\Tools\\Cache\\Manager', 'CacheManager');
}

/**
 * Environment Configuration
 * 
 * Load environment variables from .env file if it exists
 * This follows the twelve-factor app methodology
 */
function loadEnvironment(): void {
    $envFile = __DIR__ . '/.env';
    
    if (!file_exists($envFile)) {
        return;
    }
    
    $lines = file($envFile, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
    
    foreach ($lines as $line) {
        if (strpos(trim($line), '#') === 0) {
            continue; // Skip comments
        }
        
        [$name, $value] = explode('=', $line, 2);
        $name = trim($name);
        $value = trim($value);
        
        // Remove quotes if present
        if (preg_match('/^"(.*)"$/', $value, $matches)) {
            $value = $matches[1];
        } elseif (preg_match("/^'(.*)'$/", $value, $matches)) {
            $value = $matches[1];
        }
        
        if (!array_key_exists($name, $_ENV)) {
            $_ENV[$name] = $value;
            putenv("$name=$value");
        }
    }
}

// Load environment variables
loadEnvironment();

/**
 * Error Reporting Configuration
 * 
 * Set appropriate error reporting based on environment
 */
function configureErrorReporting(): void {
    $environment = $_ENV['APP_ENV'] ?? 'production';
    
    switch ($environment) {
        case 'development':
        case 'dev':
            error_reporting(E_ALL);
            ini_set('display_errors', '1');
            ini_set('display_startup_errors', '1');
            break;
            
        case 'testing':
        case 'test':
            error_reporting(E_ALL);
            ini_set('display_errors', '0');
            ini_set('log_errors', '1');
            break;
            
        case 'production':
        case 'prod':
        default:
            error_reporting(E_ERROR | E_WARNING | E_PARSE);
            ini_set('display_errors', '0');
            ini_set('log_errors', '1');
            break;
    }
}

// Configure error reporting
configureErrorReporting();

/**
 * Security Headers
 * 
 * Set security headers for all requests
 */
function setSecurityHeaders(): void {
    // Prevent MIME type sniffing
    header('X-Content-Type-Options: nosniff');
    
    // Prevent clickjacking
    header('X-Frame-Options: DENY');
    
    // Enable XSS protection
    header('X-XSS-Protection: 1; mode=block');
    
    // Control referrer information
    header('Referrer-Policy: strict-origin-when-cross-origin');
    
    // Content Security Policy (basic)
    header("Content-Security-Policy: default-src 'self'; script-src 'self' 'unsafe-inline' https://code.jquery.com https://cdnjs.cloudflare.com https://maxcdn.bootstrapcdn.com; style-src 'self' 'unsafe-inline' https://maxcdn.bootstrapcdn.com; font-src 'self' https://maxcdn.bootstrapcdn.com");
}

// Set security headers
setSecurityHeaders();

/**
 * Session Configuration
 * 
 * Configure secure session settings
 */
function configureSession(): void {
    // Session security settings
    ini_set('session.cookie_httponly', '1');
    ini_set('session.cookie_secure', '1');
    ini_set('session.use_strict_mode', '1');
    ini_set('session.cookie_samesite', 'Strict');
    
    // Set session name
    session_name('LUMINARI_SESSION');
    
    // Start session if not already started
    if (session_status() === PHP_SESSION_NONE) {
        session_start();
    }
}

// Configure and start session
configureSession();

/**
 * Timezone Configuration
 * 
 * Set default timezone for the application
 */
function configureTimezone(): void {
    $timezone = $_ENV['APP_TIMEZONE'] ?? 'UTC';
    date_default_timezone_set($timezone);
}

// Configure timezone
configureTimezone();
