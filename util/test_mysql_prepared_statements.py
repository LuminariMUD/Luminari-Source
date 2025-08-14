#!/usr/bin/env python3
"""
Test MySQL prepared statements to debug the help system issue.
This tests the exact queries that are failing in the MUD.
"""

import mysql.connector
from mysql.connector import Error
import sys

# Database connection parameters
DB_CONFIG = {
    'host': 'localhost',
    'database': 'luminari_mudprod',
    'user': 'luminari_mud',
    'password': 'vZ$eO}fD-4%7',
    'use_pure': False  # Use C extension for better compatibility with prepared statements
}

def test_help_search_query():
    """Test the main help search query that's failing"""
    print("\n" + "="*80)
    print("TESTING HELP SEARCH QUERY (the one failing for 'score')")
    print("="*80)
    
    try:
        connection = mysql.connector.connect(**DB_CONFIG)
        cursor = connection.cursor(prepared=True)
        
        # The exact query from search_help function
        query = """
            SELECT he.tag, he.entry, he.min_level, he.last_updated,
                   GROUP_CONCAT(DISTINCT CONCAT(UCASE(LEFT(hk2.keyword, 1)), LCASE(SUBSTRING(hk2.keyword, 2))) SEPARATOR ', ')
            FROM help_entries he
            LEFT JOIN help_keywords hk ON he.tag = hk.help_tag
            LEFT JOIN help_keywords hk2 ON he.tag = hk2.help_tag
            WHERE LOWER(hk.keyword) LIKE LOWER(?)
            AND he.min_level <= ?
            GROUP BY he.tag, he.entry, he.min_level, he.last_updated
            ORDER BY he.tag
        """
        
        # Test with 'score' - the keyword that's failing
        search_pattern = 'score%'
        level = 34
        
        print(f"\nExecuting prepared statement with:")
        print(f"  Pattern: '{search_pattern}'")
        print(f"  Level: {level}")
        
        cursor.execute(query, (search_pattern, level))
        
        # Fetch results
        results = cursor.fetchall()
        
        print(f"\nQuery returned {len(results)} rows")
        
        if results:
            for row in results:
                print(f"\nResult:")
                print(f"  Tag: {row[0]}")
                print(f"  Entry: {row[1][:100]}..." if len(row[1]) > 100 else f"  Entry: {row[1]}")
                print(f"  Min Level: {row[2]}")
                print(f"  Last Updated: {row[3]}")
                print(f"  Keywords: {row[4]}")
        else:
            print("\nNO RESULTS FOUND - This is the problem!")
            
            # Let's check if the data exists with a simple query
            print("\nChecking with simple non-prepared query...")
            cursor2 = connection.cursor()
            cursor2.execute("""
                SELECT he.tag, hk.keyword 
                FROM help_entries he
                JOIN help_keywords hk ON he.tag = hk.help_tag
                WHERE hk.keyword LIKE 'score%'
                LIMIT 5
            """)
            simple_results = cursor2.fetchall()
            print(f"Simple query found {len(simple_results)} results:")
            for row in simple_results:
                print(f"  {row[0]}: {row[1]}")
            cursor2.close()
            
            # Let's check with exact match
            print("\nChecking for exact keyword match...")
            cursor3 = connection.cursor()
            cursor3.execute("""
                SELECT hk.keyword, hk.help_tag
                FROM help_keywords hk
                WHERE hk.keyword = 'score'
            """)
            exact_results = cursor3.fetchall()
            print(f"Exact match query found {len(exact_results)} results:")
            for row in exact_results:
                print(f"  Keyword: '{row[0]}' -> Tag: '{row[1]}'")
            cursor3.close()
            
        cursor.close()
        
    except Error as e:
        print(f"Error: {e}")
    finally:
        if connection and connection.is_connected():
            connection.close()

def test_soundex_query():
    """Test the soundex query that's also failing"""
    print("\n" + "="*80)
    print("TESTING SOUNDEX QUERY (for 'sscore')")
    print("="*80)
    
    try:
        connection = mysql.connector.connect(**DB_CONFIG)
        cursor = connection.cursor(prepared=True)
        
        # The exact query from soundex_search_help_keywords
        query = """
            SELECT hk.help_tag, hk.keyword
            FROM help_entries he, help_keywords hk
            WHERE he.tag = hk.help_tag
            AND hk.keyword SOUNDS LIKE ?
            AND he.min_level <= ?
            ORDER BY LENGTH(hk.keyword) ASC
        """
        
        # Test with 'sscore' - should find 'score' via soundex
        search_term = 'sscore'
        level = 34
        
        print(f"\nExecuting prepared statement with:")
        print(f"  Term: '{search_term}'")
        print(f"  Level: {level}")
        
        cursor.execute(query, (search_term, level))
        
        # Fetch results
        results = cursor.fetchall()
        
        print(f"\nQuery returned {len(results)} rows")
        
        if results:
            for row in results:
                print(f"  Tag: {row[0]}, Keyword: {row[1]}")
        else:
            print("\nNO SOUNDEX MATCHES FOUND!")
            
            # Test soundex directly
            print("\nTesting SOUNDEX function directly...")
            cursor2 = connection.cursor()
            cursor2.execute("""
                SELECT 'sscore' as input, SOUNDEX('sscore') as soundex_value
                UNION ALL
                SELECT 'score', SOUNDEX('score')
                UNION ALL  
                SELECT 'skore', SOUNDEX('skore')
            """)
            soundex_results = cursor2.fetchall()
            for row in soundex_results:
                print(f"  SOUNDEX('{row[0]}') = {row[1]}")
            cursor2.close()
            
        cursor.close()
        
    except Error as e:
        print(f"Error: {e}")
    finally:
        if connection and connection.is_connected():
            connection.close()

def test_case_sensitivity():
    """Test if case sensitivity is the issue"""
    print("\n" + "="*80)
    print("TESTING CASE SENSITIVITY ISSUES")
    print("="*80)
    
    try:
        connection = mysql.connector.connect(**DB_CONFIG)
        
        # Test different case combinations
        test_cases = [
            ('score', 'lowercase'),
            ('Score', 'title case'),
            ('SCORE', 'uppercase'),
            ('score%', 'lowercase with wildcard'),
            ('Score%', 'title case with wildcard'),
        ]
        
        for keyword, description in test_cases:
            cursor = connection.cursor(prepared=True)
            
            # Simplified query to test LIKE behavior
            query = """
                SELECT COUNT(*) as count
                FROM help_keywords hk
                WHERE LOWER(hk.keyword) LIKE LOWER(?)
            """
            
            cursor.execute(query, (keyword,))
            result = cursor.fetchone()
            
            print(f"  Testing '{keyword}' ({description}): Found {result[0]} matches")
            
            cursor.close()
            
    except Error as e:
        print(f"Error: {e}")
    finally:
        if connection and connection.is_connected():
            connection.close()

def analyze_help_data():
    """Analyze the actual help data in the database"""
    print("\n" + "="*80)
    print("ANALYZING HELP DATABASE STRUCTURE")
    print("="*80)
    
    try:
        connection = mysql.connector.connect(**DB_CONFIG)
        cursor = connection.cursor()
        
        # Check help_entries table
        cursor.execute("SELECT COUNT(*) FROM help_entries")
        entry_count = cursor.fetchone()[0]
        print(f"\nhelp_entries table: {entry_count} rows")
        
        # Check help_keywords table
        cursor.execute("SELECT COUNT(*) FROM help_keywords")
        keyword_count = cursor.fetchone()[0]
        print(f"help_keywords table: {keyword_count} rows")
        
        # Check for 'score' specifically
        cursor.execute("""
            SELECT he.tag, he.min_level, COUNT(hk.keyword) as keyword_count
            FROM help_entries he
            LEFT JOIN help_keywords hk ON he.tag = hk.help_tag
            WHERE he.tag = 'score' OR hk.keyword LIKE '%score%'
            GROUP BY he.tag, he.min_level
            LIMIT 10
        """)
        
        results = cursor.fetchall()
        print(f"\nHelp entries related to 'score':")
        for row in results:
            print(f"  Tag: '{row[0]}', Min Level: {row[1]}, Keywords: {row[2]}")
        
        # Check if there's a case issue
        cursor.execute("""
            SELECT DISTINCT keyword
            FROM help_keywords
            WHERE LOWER(keyword) = 'score'
        """)
        
        results = cursor.fetchall()
        print(f"\nExact 'score' keywords (case variations):")
        for row in results:
            print(f"  '{row[0]}'")
            
        cursor.close()
        
    except Error as e:
        print(f"Error: {e}")
    finally:
        if connection and connection.is_connected():
            connection.close()

if __name__ == "__main__":
    print("MySQL Prepared Statement Debug Test")
    print("Testing the exact queries that are failing in the MUD help system")
    
    # Run all tests
    analyze_help_data()
    test_help_search_query()
    test_soundex_query()
    test_case_sensitivity()
    
    print("\n" + "="*80)
    print("TEST COMPLETE")
    print("="*80)