/* ************************************************************************
*   File: vessels_db.c                                 Part of LuminariMUD *
*  Usage: Database persistence for multi-room vessel system                *
*                                                                          *
*  All rights reserved.  See license for complete information.            *
*                                                                          *
*  LuminariMUD is based on CircleMUD, Copyright (C) 1993, 94.             *
*  CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "vessels.h"
#include "mysql.h"

/* External variables */
extern MYSQL *conn;
extern bool mysql_available;

/* Function prototypes */
void save_ship_interior(struct greyhawk_ship_data *ship);
void load_ship_interior(struct greyhawk_ship_data *ship);
void save_docking_record(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2, const char *dock_type);
void end_docking_record(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2);
void save_cargo_manifest(struct greyhawk_ship_data *ship, int cargo_room, struct obj_data *cargo);
void load_cargo_manifest(struct greyhawk_ship_data *ship);
void save_crew_roster(struct greyhawk_ship_data *ship, struct char_data *npc, const char *role);
void load_crew_roster(struct greyhawk_ship_data *ship);
int serialize_room_data(struct greyhawk_ship_data *ship, char *buffer, int buffer_size);
int deserialize_room_data(struct greyhawk_ship_data *ship, const char *data);
void cleanup_orphaned_dockings(void);

/* Save ship interior configuration to database */
void save_ship_interior(struct greyhawk_ship_data *ship)
{
    char query[MAX_STRING_LENGTH];
    char room_vnums_str[1024];
    char escaped_name[MAX_NAME_LENGTH * 2 + 1];
    char room_data_buf[4096];
    char escaped_room_data[8192];
    int i, len;
    
    if (!mysql_available || !ship) {
        return;
    }
    
    /* Build room vnums string */
    room_vnums_str[0] = '\0';
    len = 0;
    for (i = 0; i < ship->num_rooms && i < MAX_SHIP_ROOMS; i++) {
        if (i > 0) {
            len += snprintf(room_vnums_str + len, sizeof(room_vnums_str) - len, ",");
        }
        len += snprintf(room_vnums_str + len, sizeof(room_vnums_str) - len, "%d", ship->room_vnums[i]);
    }
    
    /* Escape ship name - name is an array, not pointer, so always valid */
    if (ship->name[0] != '\0') {
        mysql_real_escape_string(conn, escaped_name, ship->name, strlen(ship->name));
    } else {
        strcpy(escaped_name, "Unnamed Vessel");
    }
    
    /* Serialize room data */
    serialize_room_data(ship, room_data_buf, sizeof(room_data_buf));
    mysql_real_escape_string(conn, escaped_room_data, room_data_buf, strlen(room_data_buf));
    
    /* Build and execute query */
    snprintf(query, sizeof(query),
        "REPLACE INTO ship_interiors "
        "(ship_id, vessel_type, vessel_name, num_rooms, max_rooms, "
        "room_vnums, bridge_room, entrance_room, "
        "cargo_room1, cargo_room2, cargo_room3, cargo_room4, cargo_room5, "
        "room_data) "
        "VALUES ('%s', %d, '%s', %d, %d, '%s', %d, %d, "
        "%d, %d, %d, %d, %d, '%s')",
        ship->id, ship->vessel_type, escaped_name, ship->num_rooms, MAX_SHIP_ROOMS,
        room_vnums_str, ship->bridge_room, ship->entrance_room,
        ship->cargo_rooms[0], ship->cargo_rooms[1], ship->cargo_rooms[2],
        ship->cargo_rooms[3], ship->cargo_rooms[4], escaped_room_data);
    
    if (mysql_query(conn, query)) {
        log("SYSERR: Unable to save ship interior for ship %s: %s", ship->id, mysql_error(conn));
    } else {
        log("Info: Saved interior configuration for ship %s (%s)", ship->id, ship->name);
    }
}

/* Load ship interior configuration from database */
void load_ship_interior(struct greyhawk_ship_data *ship)
{
    MYSQL_RES *result;
    MYSQL_ROW row;
    char query[MAX_STRING_LENGTH];
    char *token;
    char room_vnums_copy[1024];
    int i;
    
    if (!mysql_available || !ship) {
        return;
    }
    
    snprintf(query, sizeof(query),
        "SELECT vessel_type, vessel_name, num_rooms, room_vnums, "
        "bridge_room, entrance_room, "
        "cargo_room1, cargo_room2, cargo_room3, cargo_room4, cargo_room5, "
        "room_data FROM ship_interiors WHERE ship_id = '%s'",
        ship->id);
    
    if (mysql_query(conn, query)) {
        log("SYSERR: Unable to load ship interior for ship %s: %s", ship->id, mysql_error(conn));
        return;
    }
    
    result = mysql_store_result(conn);
    if (!result) {
        log("SYSERR: Unable to store result for ship interior: %s", mysql_error(conn));
        return;
    }
    
    if ((row = mysql_fetch_row(result))) {
        /* Load basic data */
        ship->vessel_type = atoi(row[0]);
        /* name is an array, not pointer, so always valid */
        if (row[1] && row[1][0] != '\0') {
            strncpy(ship->name, row[1], MAX_NAME_LENGTH - 1);
            ship->name[MAX_NAME_LENGTH - 1] = '\0';
        }
        ship->num_rooms = atoi(row[2]);
        
        /* Parse room vnums */
        if (row[3]) {
            strncpy(room_vnums_copy, row[3], sizeof(room_vnums_copy) - 1);
            room_vnums_copy[sizeof(room_vnums_copy) - 1] = '\0';
            
            i = 0;
            token = strtok(room_vnums_copy, ",");
            while (token && i < MAX_SHIP_ROOMS) {
                ship->room_vnums[i++] = atoi(token);
                token = strtok(NULL, ",");
            }
        }
        
        /* Load special rooms */
        ship->bridge_room = atoi(row[4]);
        ship->entrance_room = atoi(row[5]);
        
        /* Load cargo rooms */
        for (i = 0; i < 5; i++) {
            ship->cargo_rooms[i] = atoi(row[6 + i]);
        }
        
        /* Deserialize room data */
        if (row[11]) {
            deserialize_room_data(ship, row[11]);
        }
        
        log("Info: Loaded interior configuration for ship %s", ship->id);
    } else {
        log("Info: No saved interior for ship %s, will generate new", ship->id);
    }
    
    mysql_free_result(result);
}

/* Save docking record to database */
void save_docking_record(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2, const char *dock_type)
{
    char query[MAX_STRING_LENGTH];
    
    if (!mysql_available || !ship1 || !ship2) {
        return;
    }
    
    snprintf(query, sizeof(query),
        "INSERT INTO ship_docking "
        "(ship1_id, ship2_id, dock_room1, dock_room2, dock_type, dock_status, "
        "dock_x, dock_y, dock_z) "
        "VALUES ('%s', '%s', %d, %d, '%s', 'active', %.2f, %.2f, %.2f)",
        ship1->id, ship2->id, ship1->docking_room, ship2->docking_room,
        dock_type ? dock_type : "standard",
        ship1->x, ship1->y, ship1->z);
    
    if (mysql_query(conn, query)) {
        log("SYSERR: Unable to save docking record: %s", mysql_error(conn));
    } else {
        log("Info: Recorded docking between ships %s and %s", ship1->id, ship2->id);
    }
}

/* Mark docking as completed */
void end_docking_record(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2)
{
    char query[MAX_STRING_LENGTH];
    
    if (!mysql_available || !ship1 || !ship2) {
        return;
    }
    
    snprintf(query, sizeof(query),
        "UPDATE ship_docking SET dock_status = 'completed', undock_time = NOW() "
        "WHERE ((ship1_id = '%s' AND ship2_id = '%s') OR (ship1_id = '%s' AND ship2_id = '%s')) "
        "AND dock_status = 'active'",
        ship1->id, ship2->id, ship2->id, ship1->id);
    
    if (mysql_query(conn, query)) {
        log("SYSERR: Unable to update docking record: %s", mysql_error(conn));
    } else {
        log("Info: Ended docking between ships %s and %s", ship1->id, ship2->id);
    }
}

/* Save cargo manifest entry */
void save_cargo_manifest(struct greyhawk_ship_data *ship, int cargo_room, struct obj_data *cargo)
{
    char query[MAX_STRING_LENGTH];
    char escaped_name[MAX_NAME_LENGTH * 2 + 1];
    
    if (!mysql_available || !ship || !cargo) {
        return;
    }
    
    mysql_real_escape_string(conn, escaped_name, cargo->short_description, 
                           strlen(cargo->short_description));
    
    snprintf(query, sizeof(query),
        "INSERT INTO ship_cargo_manifest "
        "(ship_id, cargo_room, item_vnum, item_name, item_count, item_weight) "
        "VALUES ('%s', %d, %d, '%s', %d, %d)",
        ship->id, cargo_room, GET_OBJ_VNUM(cargo), escaped_name,
        1, GET_OBJ_WEIGHT(cargo));
    
    if (mysql_query(conn, query)) {
        log("SYSERR: Unable to save cargo manifest: %s", mysql_error(conn));
    }
}

/* Load cargo manifest for a ship */
void load_cargo_manifest(struct greyhawk_ship_data *ship)
{
    MYSQL_RES *result;
    MYSQL_ROW row;
    char query[MAX_STRING_LENGTH];
    struct obj_data *cargo;
    room_rnum cargo_room;
    obj_rnum obj_num;
    
    if (!mysql_available || !ship) {
        return;
    }
    
    snprintf(query, sizeof(query),
        "SELECT cargo_room, item_vnum, item_count FROM ship_cargo_manifest "
        "WHERE ship_id = '%s'",
        ship->id);
    
    if (mysql_query(conn, query)) {
        log("SYSERR: Unable to load cargo manifest: %s", mysql_error(conn));
        return;
    }
    
    result = mysql_store_result(conn);
    if (!result) {
        return;
    }
    
    while ((row = mysql_fetch_row(result))) {
        cargo_room = real_room(atoi(row[0]));
        obj_num = real_object(atoi(row[1]));
        
        if (cargo_room != NOWHERE && obj_num != NOTHING) {
            cargo = read_object(obj_num, REAL);
            if (cargo) {
                obj_to_room(cargo, cargo_room);
                log("Info: Loaded cargo item %d to room %d on ship %s",
                    GET_OBJ_VNUM(cargo), world[cargo_room].number, ship->id);
            }
        }
    }
    
    mysql_free_result(result);
}

/* Save crew roster entry */
void save_crew_roster(struct greyhawk_ship_data *ship, struct char_data *npc, const char *role)
{
    char query[MAX_STRING_LENGTH];
    char escaped_name[MAX_NAME_LENGTH * 2 + 1];
    
    /* IS_PC doesn't exist, use !IS_NPC instead */
    if (!mysql_available || !ship || !npc || !IS_NPC(npc)) {
        return;
    }
    
    mysql_real_escape_string(conn, escaped_name, GET_NAME(npc), strlen(GET_NAME(npc)));
    
    snprintf(query, sizeof(query),
        "INSERT INTO ship_crew_roster "
        "(ship_id, npc_vnum, npc_name, crew_role, assigned_room) "
        "VALUES ('%s', %d, '%s', '%s', %d)",
        ship->id, GET_MOB_VNUM(npc), escaped_name,
        role ? role : "crew", IN_ROOM(npc));
    
    if (mysql_query(conn, query)) {
        log("SYSERR: Unable to save crew roster: %s", mysql_error(conn));
    }
}

/* Load crew roster for a ship */
void load_crew_roster(struct greyhawk_ship_data *ship)
{
    MYSQL_RES *result;
    MYSQL_ROW row;
    char query[MAX_STRING_LENGTH];
    struct char_data *npc;
    mob_rnum mob_num;
    room_rnum target_room;
    
    if (!mysql_available || !ship) {
        return;
    }
    
    snprintf(query, sizeof(query),
        "SELECT npc_vnum, assigned_room, crew_role FROM ship_crew_roster "
        "WHERE ship_id = '%s' AND status = 'active'",
        ship->id);
    
    if (mysql_query(conn, query)) {
        log("SYSERR: Unable to load crew roster: %s", mysql_error(conn));
        return;
    }
    
    result = mysql_store_result(conn);
    if (!result) {
        return;
    }
    
    while ((row = mysql_fetch_row(result))) {
        mob_num = real_mobile(atoi(row[0]));
        target_room = real_room(atoi(row[1]));
        
        if (mob_num != NOBODY && target_room != NOWHERE) {
            npc = read_mobile(mob_num, REAL);
            if (npc) {
                char_to_room(npc, target_room);
                /* Set ship loyalty here if needed */
                log("Info: Loaded crew member %s to ship %s",
                    GET_NAME(npc), ship->id);
            }
        }
    }
    
    mysql_free_result(result);
}

/* Serialize room connection data */
int serialize_room_data(struct greyhawk_ship_data *ship, char *buffer, int buffer_size)
{
    int i, len = 0;
    
    if (!ship || !buffer) {
        return 0;
    }
    
    /* Simple format: connection_count|from:to:dir:hatch:locked|... */
    len = snprintf(buffer, buffer_size, "%d", ship->num_connections);
    
    for (i = 0; i < ship->num_connections && i < MAX_SHIP_CONNECTIONS; i++) {
        len += snprintf(buffer + len, buffer_size - len, "|%d:%d:%d:%d:%d",
            ship->connections[i].from_room,
            ship->connections[i].to_room,
            ship->connections[i].direction,
            ship->connections[i].is_hatch ? 1 : 0,
            ship->connections[i].is_locked ? 1 : 0);
    }
    
    return len;
}

/* Deserialize room connection data */
int deserialize_room_data(struct greyhawk_ship_data *ship, const char *data)
{
    char data_copy[4096];
    char *token;
    int i = 0;
    int conn_count;
    
    if (!ship || !data) {
        return 0;
    }
    
    strncpy(data_copy, data, sizeof(data_copy) - 1);
    data_copy[sizeof(data_copy) - 1] = '\0';
    
    /* Parse format: connection_count|from:to:dir:hatch:locked|... */
    /* Simple parsing for C89 compatibility */
    if (sscanf(data_copy, "%d", &conn_count) == 1) {
        ship->num_connections = conn_count;
    }
    
    /* Find first | separator */
    token = strchr(data_copy, '|');
    if (!token) {
        return 0;
    }
    token++; /* Skip the | */
    
    /* Parse each connection */
    while (token && *token && i < MAX_SHIP_CONNECTIONS) {
        int from, to, dir, hatch, locked;
        
        if (sscanf(token, "%d:%d:%d:%d:%d", &from, &to, &dir, &hatch, &locked) == 5) {
            ship->connections[i].from_room = from;
            ship->connections[i].to_room = to;
            ship->connections[i].direction = dir;
            ship->connections[i].is_hatch = hatch ? TRUE : FALSE;
            ship->connections[i].is_locked = locked ? TRUE : FALSE;
            i++;
        }
        
        /* Find next | separator */
        token = strchr(token, '|');
        if (token) {
            token++; /* Skip the | */
        }
    }
    
    return i;
}

/* Clean up orphaned docking records */
void cleanup_orphaned_dockings(void)
{
    char query[MAX_STRING_LENGTH];
    
    if (!mysql_available) {
        return;
    }
    
    /* Call stored procedure to clean up old dockings */
    snprintf(query, sizeof(query), "CALL cleanup_orphaned_dockings()");
    
    if (mysql_query(conn, query)) {
        log("SYSERR: Unable to cleanup orphaned dockings: %s", mysql_error(conn));
    } else {
        log("Info: Cleaned up orphaned docking records");
    }
}

/* Initialize vessel database tables */
void init_vessel_db(void)
{
    if (!mysql_available) {
        log("Info: MySQL not available, vessel persistence disabled");
        return;
    }
    
    /* Clean up any orphaned dockings on startup */
    cleanup_orphaned_dockings();
    
    log("Info: Vessel database persistence initialized");
}