#include "parser/mavlink_parser.h"

MavlinkParser::MavlinkParser(QObject* parent) : QObject(parent) {}

void MavlinkParser::processData(const QByteArray& data) {
    for (char c : data) {
        if (mavlink_parse_char(MAVLINK_COMM_0, uint8_t(c), &msg, &status)) {
            // Decode known important messages
            if (msg.msgid == MAVLINK_MSG_ID_GLOBAL_POSITION_INT) {
                mavlink_global_position_int_t pos;
                mavlink_msg_global_position_int_decode(&msg, &pos);
                emit messageParsed(MAVLINK_MSG_ID_GLOBAL_POSITION_INT, &pos);
            }
            else if (msg.msgid == MAVLINK_MSG_ID_VFR_HUD) {
                mavlink_vfr_hud_t hud;
                mavlink_msg_vfr_hud_decode(&msg, &hud);
                emit messageParsed(MAVLINK_MSG_ID_VFR_HUD, &hud);
            }
            else if (msg.msgid == MAVLINK_MSG_ID_SYS_STATUS) {
                mavlink_sys_status_t status_msg;
                mavlink_msg_sys_status_decode(&msg, &status_msg);
                emit messageParsed(MAVLINK_MSG_ID_SYS_STATUS, &status_msg);
            }
            else {
                // For other messages, pass generic
                emit messageParsed(msg.msgid, &msg.payload64[0]);
            }
        }
    }
}
