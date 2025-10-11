#include "CharFlow.hpp"

#include <cstring>
#include <string>

#include "interface/ragnarok/dto/ActorDTO.hpp"
#include "interface/ragnarok/dto/CharacterDTO.hpp"
#include "interface/ragnarok/dto/MapDTO.hpp"
#include "interface/ragnarok/dto/SystemDTO.hpp"
#include "interface/ragnarok/mappers/ActorMapper.hpp"
#include "interface/ragnarok/mappers/CharacterMapper.hpp"
#include "interface/ragnarok/mappers/MapMapper.hpp"
#include "interface/ragnarok/mappers/SystemMapper.hpp"
#include "interface/ragnarok/model/SpawnTable.hpp"
#include "interface/ragnarok/protocol/Codec.hpp"
#include "interface/ragnarok/protocol/Coords.hpp"
#include "shared/Hex.hpp"

namespace arkan
{
namespace thanatos
{
namespace interface
{
namespace ro
{
namespace charflow
{

// =========================================================================
// Aliases for clean code
// クリーンなコードのためのエイリアス
// =========================================================================
namespace dto = arkan::thanatos::interface::ro::dto;
namespace mappers = arkan::thanatos::interface::ro::mappers;
using arkan::thanatos::interface::ro::model::safeSpawnFor;
using protocol::tick_ms;
// =========================================================================

/* -------------------- Constructor -------------------- */
/* -------------------- コンストラクタ -------------------- */
CharFlow::CharFlow(CharConfig& cfg, CharState& st, SendFn send, LogFn log)
    : cfg_(cfg), st_(st), send_(std::move(send)), log_(std::move(log))
{
}

/* ----------------- Opcode Classifiers ------------- */
/* ----------------- オペコード分類器 ------------- */
bool CharFlow::isSelectServerOpcode(uint16_t op)
{
    // Checks if the opcode corresponds to the client selecting a character server.
    // オペコードがクライアントのキャラクターサーバー選択に対応するかどうかをチェックします。
    switch (op)
    {
        case 0x0065:
        case 0x0275:
        case 0x0825:
        case 0x0B1D:
        case 0x1DD6:
        case 0x2B0F:
        case 0x0101:
            return true;
        default:
            return false;
    }
}

bool CharFlow::isCharSelectOpcode(uint16_t op)
{
    // Checks if the opcode corresponds to the client selecting a character from the list.
    // オペコードがクライアントのキャラクターリストからのキャラクター選択に対応するかどうかをチェックします。
    switch (op)
    {
        case 0x0066:
        case 0x08A9:
        case 0x0B19:
        case 0x1DD7:
        case 0x2B10:
            return true;
        default:
            return false;
    }
}

bool CharFlow::isEnterOpcode(uint16_t op)
{
    // Checks if the opcode is a known map entry request from the client.
    // オペコードがクライアントからの既知のマップ入場リクエストであるかどうかをチェックします。
    return (op == 0x2844 || op == 0x0436 || op == 0x0072 || op == 0x009B);
}

/* -------------------- Main Packet Handler -------------------- */
/* -------------------- メインパケットハンドラ -------------------- */
void CharFlow::handle(uint16_t opcode, const uint8_t* data, size_t len)
{
    // Robust fallback: treat the FIRST packet after redirect as a map login request.
    // 堅牢なフォールバック：リダイレクト後の最初のパケットをマップログインリクエストとして扱います。
    if (expecting_map_login_)
    {
        if (isEnterOpcode(opcode))
        {
            onEnter(opcode, data, len);
            return;
        }
        if (first_packet_after_redirect_)
        {
            first_packet_after_redirect_ = false;
            log_(std::string("enter fallback: treating first packet as MAP-LOGIN (opcode=0x") +
                 hex::hex16(opcode) + ", len=" + std::to_string(len) + ")");
            onEnter(opcode, data, len);
            return;
        }
    }

    if (isSelectServerOpcode(opcode))
    {
        onServerSelected(data, len);
        return;
    }
    if (opcode == 0x09A1)  // Request for character list
    {
        onCharListReq();
        return;
    }
    if (isCharSelectOpcode(opcode))
    {
        onCharSelected(data, len);
        return;
    }

    log_(std::string("unhandled opcode: 0x") + hex::hex16(opcode) + " (len=" + std::to_string(len) +
         ")");
}

void CharFlow::armExpectingMapLogin()
{
    // Arms the state machine to expect a map login packet next. Called after a redirect.
    // 次にマップログインパケットを期待するようにステートマシンを準備します。リダイレクト後に呼び出されます。
    expecting_map_login_ = true;
    first_packet_after_redirect_ = true;
    log_("armed expecting_map_login_ from CharHandler");
}

void CharFlow::setFixedSpawn(std::string map, std::uint16_t x, std::uint16_t y, std::uint8_t dir)
{
    // Sets a specific spawn point, overriding defaults.
    // 日本語: デフォルトを上書きして、特定のスポーンポイントを設定します。
    spawn_override_ = SpawnOverride{std::move(map), x, y, dir};
    log_("fixed spawn set");
}

/* ----------------- Character Flow Steps -------------- */
/* ----------------- キャラクターフローのステップ -------------- */

void CharFlow::onServerSelected(const uint8_t* data, size_t len)
{
    log_("server selected");
    if (len >= 4) std::memcpy(st_.accountID.data(), data, 4);

    // Send the character list preamble and a single character slot.
    // キャラクターリストのプリアンブルと単一のキャラクタースロットを送信します。
    dto::PreambleAccountID preamble_aid_dto;
    preamble_aid_dto.account_id = st_.accountID;
    send_(mappers::to_packet(preamble_aid_dto));

    send_(mappers::to_packet(dto::PreambleCommand082D{}));

    dto::PreambleCommand09A0 preamble_09a0_dto;
    preamble_09a0_dto.value = 1;
    send_(mappers::to_packet(preamble_09a0_dto));

    dto::CharListInfo char_list_dto;
    char_list_dto.char_id = 1001;
    char_list_dto.slot = 0;
    char_list_dto.name = "Thanatos";
    char_list_dto.map_name = cfg_.initialMap;
    char_list_dto.is_male = (cfg_.sex != 0);
    send_(mappers::to_packet(char_list_dto));

    awaiting_charlist_req_ = true;
}

void CharFlow::onCharListReq()
{
    // Some clients request the character list again. We resend it.
    // 一部のクライアントはキャラクターリストを再度要求します。再送します。
    if (awaiting_charlist_req_)
    {
        log_("got 09A1 -> resend 099D");
        awaiting_charlist_req_ = false;

        // Reuse the same DTO creation logic.
        // 同じDTO作成ロジックを再利用します。
        dto::CharListInfo char_list_dto;
        char_list_dto.char_id = 1001;
        char_list_dto.slot = 0;
        char_list_dto.name = "Thanatos";
        char_list_dto.map_name = cfg_.initialMap;
        char_list_dto.is_male = (cfg_.sex != 0);
        send_(mappers::to_packet(char_list_dto));
    }
    else
    {
        log_("got 09A1 but not awaiting; ignored");
    }
}

void CharFlow::onCharSelected(const uint8_t* data, size_t len)
{
    log_("character selected -> redirect");
    if (len >= 4) std::memcpy(st_.selectedCharID.data(), data, 4);

    // Create a DTO for the map server redirect information.
    // マップサーバーリダイレクト情報のためのDTOを作成します。
    dto::RedirectInfo redirect_dto;
    redirect_dto.char_id = st_.selectedCharID;
    redirect_dto.map_name = cfg_.initialMap;
    redirect_dto.ip = cfg_.mapIp;
    redirect_dto.port = cfg_.mapPortLE;

    // Send the short version of the redirect packet (for older clients).
    // 短いバージョン（古いクライアント向け）のリダイレクトパケットを送信します。
    redirect_dto.use_full_redirect = false;
    send_(mappers::to_packet(redirect_dto));

    // Fill in extra data and send the full version (for modern clients).
    // 追加データを入力し、完全なバージョン（現代のクライアント向け）を送信します。
    redirect_dto.account_id = st_.accountID;
    redirect_dto.session_a = {0x00, 0x5E, 0xD0, 0xB2};
    redirect_dto.session_b = {0xFF, 0x00, 0x00, 0x00};
    redirect_dto.sex = cfg_.sex;
    redirect_dto.use_full_redirect = true;
    send_(mappers::to_packet(redirect_dto));

    setFixedSpawn(cfg_.initialMap, 139, 261, 0);
    armExpectingMapLogin();
}

void CharFlow::onEnter(uint16_t opcode, const uint8_t* data, size_t len)
{
    (void)opcode;
    expecting_map_login_ = false;
    first_packet_after_redirect_ = false;

    // Calculate safe spawn coordinates, allowing for overrides or client-provided
    // 日本語: 上書きやクライアント指定の位置を考慮し、安全なスポーン座標を計算します。
    std::string map = cfg_.initialMap;
    uint16_t x = safeSpawnFor(cfg_.initialMap).x;
    uint16_t y = safeSpawnFor(cfg_.initialMap).y;
    uint8_t dir = safeSpawnFor(cfg_.initialMap).dir;

    if (spawn_override_)
    {
        map = spawn_override_->map;
        x = spawn_override_->x;
        y = spawn_override_->y;
        dir = spawn_override_->dir;
        spawn_override_.reset();
        log_("MAP-LOGIN (fixed) -> map=" + map + " x=" + std::to_string(x) +
             " y=" + std::to_string(y) + " dir=" + std::to_string((int)dir));
    }
    else if (len >= 19)
    {
        auto a3 = protocol::decodeA3(data + 16);
        a3 = protocol::clamp1023(a3);
        if (a3.x || a3.y)
        {
            x = a3.x;
            y = a3.y;
            dir = a3.dir;
        }
        log_("MAP-LOGIN (client A3) -> x=" + std::to_string(x) + " y=" + std::to_string(y) +
             " dir=" + std::to_string((int)dir));
    }

    // Send the standard sequence of packets to let the client enter the map.
    // クライアントがマップに入れるように、標準的なパケットシーケンスを送信します。

    // Stable sequence
    dto::SyncInfo sync_dto{st_.accountID};
    send_(mappers::to_packet(sync_dto));

    dto::MapLoadedInfo map_loaded_dto{tick_ms(), x, y, dir};
    send_(mappers::to_packet(map_loaded_dto));

    dto::AcceptEnterInfo accept_enter_dto{x, y, dir};
    send_(mappers::to_packet(accept_enter_dto));

    dto::WarpInfo warp_dto{cfg_.initialMap, x, y};
    send_(mappers::to_packet(warp_dto));

    // Minimal HUD
    dto::AttackRangeInfo attack_range_dto{1};
    send_(mappers::to_packet(attack_range_dto));

    send_(mappers::to_packet(dto::CharacterStatsInfo{}));  // Sends a DTO with default values

    dto::HpSpUpdateInfo hp_sp_dto{40, 40, 9, 11};
    send_(mappers::to_packet(hp_sp_dto));

    send_(mappers::to_packet(dto::LoadConfirm{}));  // Empty DTO for signaling packets

    dto::LookToInfo look_to_dto{st_.accountID, 4};
    send_(mappers::to_packet(look_to_dto));

    dto::SystemChatMessage chat_dto{"Welcome to Thanatos!"};
    send_(mappers::to_packet(chat_dto));

    dto::ActorDisplayInfo actor_info_dto{st_.accountID, "Thanatos"};
    send_(mappers::to_packet(actor_info_dto));

    dto::ActorNameInfo actor_name_dto{st_.accountID, "Thanatos"};
    send_(mappers::to_packet(actor_name_dto));
}

}  // namespace charflow
}  // namespace ro
}  // namespace interface
}  // namespace thanatos
}  // namespace arkan