#include "SFEPanel.hpp"
#include "BpNode.hpp"
#include "bpcommon.hpp"

namespace sfe {
/**
 * @note
 * 1. 启动游戏，显示迷宫，根据迷宫编写对应的程序
 *      * 设置所有节点断点
 *      * 执行begin事件
 * 2. 运行程序，执行结束，判断是否走出迷宫，（弹窗提醒）
 *      * 启动循环，待一次循环结束后
 *      * 接收节点发送的消息，判断游戏是否通关
 *      * 弹窗提醒
 *      * 发送停止请求
 */
class panel_maze : public SFEPanel {
    double _last_ts = 0.0;
    bool _runing = false;
    bool _req_start = false;
    double _run_speed = 0.5; // s
    std::string _state = "";
    bool _stop = false;
public:
    virtual bool Init() override {
        return true;
    }

    virtual void Update() override {
        ImGui::Begin(PanelName().c_str());

        if (ImGui::Button("Start")) {
            Json::Value v;
            v["command"] = "breakpoint_cur_graph";
            v["type"] = "req";
            v["id"] = "all";
            v["set"] = true;
            SendMessage("all", v);

            v.clear();
            v["command"] = "debug_cur_graph";
            v["type"] = "req";
            v["stage"] = "start";
            SendMessage("all", v);
            _req_start = true;
        } ImGui::SameLine();
        if (ImGui::Button("Run")) {
            _runing = true;
            _last_ts = bp::BpCommon::GetTimestamp();
        }

        ImGui::End();

        // run
        if (_runing) {
            auto now = bp::BpCommon::GetTimestamp();
            if (now - _last_ts < _run_speed) {
                return;
            }
            _last_ts = now;
            // 间隔时间到就执行一个节点
            Json::Value v;
            v["command"] = "debug_cur_graph";
            v["type"] = "req";
            v["stage"] = "continue";
            SendMessage("all", v);
        }
        if (_stop) {
            ImGui::OpenPopup("State");
        }
        if (!_runing && _stop) {
            if (ImGui::BeginPopupModal("State", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::TextUnformatted(_state.c_str());
                if (ImGui::Button("OK", ImVec2(120, 0))) { 
                    _stop = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::EndPopup();
            }
        }
    }

    virtual void Exit() override {

    }

    virtual void OnMessage(const SFEMessage& msg) override {
        if (msg.json_msg.isNull()) {
            return;
        }
        auto& jmsg = msg.json_msg;
        auto cmd = jmsg["command"].asString();
        if (cmd == "debug_cur_graph" && jmsg["type"].asString() == "resp") {
            if (jmsg["stage"] == "start" && _req_start) {
                // 启动调试，初始化开局迷宫
                Json::Value v;
                v["command"] = "debug_cur_graph";
                v["type"] = "req";
                v["stage"] = "continue";
                SendMessage("all", v);
                SendMessage("all", v);
                _req_start = false;
            } else if (jmsg["stage"] == "continue" && _runing) {
                // 执行一次就结束
                if (jmsg["run_state"].asInt() == (int)bp::BpNodeRunState::BP_RUN_OK) {
                    Json::Value v;
                    v["command"] = "debug_cur_graph";
                    v["type"] = "req";
                    v["stage"] = "stop";
                    SendMessage("all", v);
                    _runing = false;
                }
            } else if (jmsg["stage"] == "stop") {
                _stop = true;
                LOG(INFO) << "stop debug";
            }
        } else if (cmd == "maze_state") {
            _state = jmsg["state"].asString();
        }
    }
};

} // namespace sfe

extern "C" std::shared_ptr<sfe::SFEPanel> create_panel(const std::string& panel_type) {
    return std::make_shared<sfe::panel_maze>();
}
