#!/usr/bin/env python3
"""
LuminariMUD Terrain Bridge API Client
Simple Python client library for connecting to the terrain bridge API server
"""

import socket
import json
import sys
import time


class LuminariTerrainAPI:
    """Python client for LuminariMUD Terrain Bridge API"""
    
    def __init__(self, host='localhost', port=8182, timeout=5.0):
        """
        Initialize the terrain API client
        
        Args:
            host (str): Server hostname (default: localhost)
            port (int): Server port (default: 8182)
            timeout (float): Connection timeout in seconds (default: 5.0)
        """
        self.host = host
        self.port = port
        self.timeout = timeout
    
    def _send_request(self, request_data):
        """
        Send a request to the terrain API server
        
        Args:
            request_data (dict): Request data to send
            
        Returns:
            dict: Response from server or error message
        """
        try:
            # Create socket connection
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(self.timeout)
            sock.connect((self.host, self.port))
            
            # Send JSON request with newline terminator
            request_json = json.dumps(request_data)
            sock.send((request_json + '\n').encode('utf-8'))
            
            # Receive response
            response_data = b''
            while True:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                response_data += chunk
                if b'\n' in response_data:
                    break
            
            sock.close()
            
            # Parse JSON response
            response_str = response_data.decode('utf-8').strip()
            return json.loads(response_str)
            
        except socket.timeout:
            return {"error": "Connection timeout", "success": False}
        except socket.error as e:
            return {"error": f"Socket error: {str(e)}", "success": False}
        except json.JSONDecodeError as e:
            return {"error": f"Invalid JSON response: {str(e)}", "success": False}
        except Exception as e:
            return {"error": f"Unexpected error: {str(e)}", "success": False}
    
    def ping(self):
        """
        Test server connectivity
        
        Returns:
            dict: Server response with pong message and server info
        """
        request = {"command": "ping"}
        return self._send_request(request)
    
    def get_terrain(self, x, y):
        """
        Get terrain data for a single coordinate
        
        Args:
            x (int): X coordinate
            y (int): Y coordinate
            
        Returns:
            dict: Terrain data including elevation, moisture, temperature, sector type
        """
        request = {
            "command": "get_terrain",
            "x": x,
            "y": y
        }
        return self._send_request(request)
    
    def get_terrain_batch(self, x_min, y_min, x_max, y_max):
        """
        Get terrain data for a range of coordinates
        
        Args:
            x_min (int): Minimum X coordinate
            y_min (int): Minimum Y coordinate  
            x_max (int): Maximum X coordinate
            y_max (int): Maximum Y coordinate
            
        Returns:
            dict: Batch terrain data with array of coordinate data
        """
        total_coords = (x_max - x_min + 1) * (y_max - y_min + 1)
        if total_coords > 1000:
            return {
                "error": "Batch too large (max 1000 coordinates)", 
                "success": False,
                "requested": total_coords
            }
        
        request = {
            "command": "get_terrain_batch",
            "params": {
                "x_min": x_min,
                "y_min": y_min,
                "x_max": x_max,
                "y_max": y_max
            }
        }
        return self._send_request(request)
    
    def get_static_rooms_list(self, limit=None):
        """
        Get static room list with basic info (vnum, name, coordinates, zone)
        
        Args:
            limit (int, optional): Maximum number of rooms to return (default: 1000)
            
        Returns:
            dict: Basic room data for all static rooms
        """
        request = {"command": "get_static_rooms_list"}
        if limit:
            request["limit"] = limit
        return self._send_request(request)
    
    def get_room_details(self, vnum):
        """
        Get detailed room information including exits, descriptions, etc.
        
        Args:
            vnum (int): Room virtual number
            
        Returns:
            dict: Complete room data with exits, zones, and terrain information
        """
        request = {
            "command": "get_room_details",
            "vnum": vnum
        }
        return self._send_request(request)


def test_terrain_api():
    """Test function to verify the terrain API is working"""
    
    print("=" * 60)
    print("LuminariMUD Terrain Bridge API Test")
    print("=" * 60)
    
    # Create API client
    api = LuminariTerrainAPI()
    
    # Test 1: Ping server
    print("\n1. Testing server connectivity...")
    result = api.ping()
    if result.get("success"):
        print(f"✓ Server responded: {result.get('message')}")
        print(f"  Server time: {result.get('server_time')}")
        print(f"  Uptime: {result.get('uptime')} seconds")
    else:
        print(f"✗ Ping failed: {result.get('error')}")
        return False
    
    # Test 2: Single coordinate
    print("\n2. Testing single coordinate lookup...")
    test_coords = [(0, 0), (100, -50), (500, 300), (-200, 400)]
    
    for x, y in test_coords:
        result = api.get_terrain(x, y)
        if result.get("success"):
            data = result.get("data", {})
            print(f"✓ ({x:4d}, {y:4d}): "
                  f"elev={data.get('elevation', 'N/A'):3d}, "
                  f"temp={data.get('temperature', 'N/A'):3d}, "
                  f"moist={data.get('moisture', 'N/A'):3d}, "
                  f"sector={data.get('sector_type', 'N/A'):2d} ({data.get('sector_name', 'unknown')})")
        else:
            print(f"✗ ({x:4d}, {y:4d}): {result.get('error')}")
    
    # Test 3: Batch request
    print("\n3. Testing batch coordinate lookup...")
    result = api.get_terrain_batch(0, 0, 4, 4)  # 5x5 grid = 25 coordinates
    
    if result.get("success"):
        data = result.get("data", [])
        count = result.get("count", 0)
        print(f"✓ Batch request successful: {count} coordinates")
        
        # Show first few results
        print("  Sample data (first 5 coordinates):")
        for i, coord in enumerate(data[:5]):
            print(f"    ({coord.get('x'):2d}, {coord.get('y'):2d}): "
                  f"elev={coord.get('elevation'):3d}, "
                  f"sector={coord.get('sector_type'):2d}")
    else:
        print(f"✗ Batch request failed: {result.get('error')}")
    
    # Test 4: Error handling
    print("\n4. Testing error handling...")
    
    # Test invalid coordinates
    result = api.get_terrain(9999, 9999)
    if not result.get("success"):
        print(f"✓ Invalid coordinates handled: {result.get('error')}")
    else:
        print("✗ Invalid coordinates should have failed")
    
    # Test too large batch
    result = api.get_terrain_batch(0, 0, 50, 50)  # 51x51 = 2601 coordinates
    if not result.get("success"):
        print(f"✓ Large batch handled: {result.get('error')}")
    else:
        print("✗ Large batch should have failed")
    
    # Test 5: Static rooms list
    print("\n5. Testing static rooms list...")
    result = api.get_static_rooms_list(limit=50)  # Start with a small limit
    
    if result.get("success"):
        total_rooms = result.get("total_rooms", 0)
        rooms_data = result.get("data", [])
        print(f"✓ Static rooms list successful: {total_rooms} rooms loaded")
        
        # Show sample room data
        if rooms_data:
            sample_room = rooms_data[0]
            print("  Sample room data:")
            print(f"    VNUM: {sample_room.get('vnum')}")
            print(f"    Name: {sample_room.get('name', 'N/A')[:50]}")
            print(f"    Coordinates: ({sample_room.get('x', 0)}, {sample_room.get('y', 0)})")
            print(f"    Sector: {sample_room.get('sector_type')}")
            print(f"    Zone: {sample_room.get('zone_name', 'N/A')} (VNUM: {sample_room.get('zone_vnum')})")
            
            # Test 6: Room details for the first room
            print("\n6. Testing room details...")
            test_vnum = '1000123'  # Use specific test VNUM
            detail_result = api.get_room_details(test_vnum)
            
            if detail_result.get("success"):
                room_detail = detail_result.get("data", {})
                print(f"✓ Room details successful for room {test_vnum}")
                
                # Handle case where data might be a list or dict
                if isinstance(room_detail, list):
                    if room_detail:
                        room_detail = room_detail[0]  # Take first item if it's a list
                    else:
                        room_detail = {}
                
                print(f"  Description: {room_detail.get('description', 'N/A')[:100]}...")
                
                # Show exits
                exits = room_detail.get('exits', [])
                if exits:
                    print(f"  Exits ({len(exits)}):")
                    for exit_info in exits[:3]:  # Show first 3 exits
                        print(f"    {exit_info.get('direction')}: to room {exit_info.get('to_room_vnum')} "
                              f"({exit_info.get('to_room_sector_type')})")
                else:
                    print("  No exits found")
                    
                # Show room flags
                flags = [room_detail.get(f'room_flags_{i}', 0) for i in range(4)]
                print(f"  Room flags: [{', '.join(map(str, flags))}]")
            else:
                print(f"✗ Room details failed: {detail_result.get('error')}")
        
        # Quick statistics
        if len(rooms_data) > 1:
            zones = set()
            sectors = {}
            coords_count = 0
            
            for room in rooms_data:
                zone_vnum = room.get('zone_vnum')
                if zone_vnum and zone_vnum != -1:
                    zones.add(zone_vnum)
                
                sector = room.get('sector_type')
                sectors[sector] = sectors.get(sector, 0) + 1
                
                if room.get('x', 0) != 0 or room.get('y', 0) != 0:
                    coords_count += 1
            
            print(f"  Sample stats ({total_rooms} rooms total):")
            print(f"    Unique zones: {len(zones)}")
            print(f"    Rooms with coordinates: {coords_count}")
            print(f"    Most common sector: {max(sectors.items(), key=lambda x: x[1])[0] if sectors else 'N/A'}")
    else:
        print(f"✗ Static rooms list failed: {result.get('error')}")

    print("\n" + "=" * 60)
    print("Terrain API test completed!")
    print("=" * 60)
    
    return True


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "test":
        test_terrain_api()
    else:
        print("LuminariMUD Terrain Bridge API Client")
        print("Usage:")
        print("  python3 terrain_api_client.py test    # Run test suite")
        print("  python3 terrain_api_client.py         # Show this help")
        print()
        print("Example usage in code:")
        print("  from terrain_api_client import LuminariTerrainAPI")
        print("  api = LuminariTerrainAPI()")
        print("  result = api.get_terrain(100, 200)")
        print("  print(result)")
