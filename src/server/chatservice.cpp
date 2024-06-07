#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>
using namespace std;
using namespace muduo;

//获取单例对象的接口函数
ChatService *ChatService::instance() {
    static ChatService service;
    return &service;
}

//注册消息以及对应的Handler回调操作
ChatService::ChatService() {
    //用户基本业务管理相关事件处理回调注册
    msgHandlerMap_.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    msgHandlerMap_.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    msgHandlerMap_.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    msgHandlerMap_.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 群组业务管理相关事件处理回调注册
    msgHandlerMap_.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    msgHandlerMap_.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    msgHandlerMap_.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
}

void ChatService::reset() {
    //把online用户的状态全部改成offline
    userModel_.resetState();
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = msgHandlerMap_.find(msgid);
    if (it == msgHandlerMap_.end())
    {
        // 返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp) {
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        };
    }
    else
    {
        return msgHandlerMap_[msgid];
    }
}

void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = userModel_.query(id);
    if (user.getId() == id && user.getPwd() == pwd) {
        if (user.getState() == "online") {
            //用户已经登陆，不允许重复登录
            json responce;
            responce["msgid"] = LOGIN_MSG_ACK;
            responce["errno"] = 2;
            responce["errmsg"] = "this account is using, input another!";
            conn->send(responce.dump());
        } else {
            //登陆成功，记录用户连接信息
            {
                lock_guard<mutex> lock(connMutex_);
                userConnMap_.insert({id, conn});
            }
            //id登陆成功，更新用户状态信息 state offline=>online
            user.setState("online");
            userModel_.updateState(user);

            json responce;
            responce["msgid"] = LOGIN_MSG_ACK;
            responce["errno"] = 0;
            responce["id"] = user.getId();
            responce["name"] = user.getName();

            //查询用户是否有离线消息，有的话直接带过去
            vector<string> vec = offlineMsgModel_.query(id);
            if (!vec.empty()) {
                responce["offlinemsg"] = vec;
                //读取该用户的离线消息厚爱，把该用户的所有离线消息删除掉
                offlineMsgModel_.remove(id);
            }

            //查询该用户的好友信息并返回
            vector<User> userVec = friendModel_.query(id);
            if (!userVec.empty()) {
                vector<string> vec2;
                for (User &user : userVec) {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                responce["friends"] = vec2;
            }

            conn->send(responce.dump());
        }
    } else {
        //用户不存在，用户存在但是密码错误，登陆失败
        json responce;
        responce["msgid"] = LOGIN_MSG_ACK;
        responce["errno"] = 1;
        responce["errmsg"] = "id or password is invalid!";
        conn->send(responce.dump());
    }
}

void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = userModel_.insert(user);
    if (state) {
        //注册成功
        json responce;
        responce["msgid"] = REG_MSG_ACK;
        responce["errno"] = 0;
        responce["id"] = user.getId();
        conn->send(responce.dump());
    } else {
        json responce;
        responce["msgid"] = REG_MSG_ACK;
        responce["errno"] = 1;
        conn->send(responce.dump());
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int toid = js["toid"].get<int>();

    {
        lock_guard<mutex> lock(connMutex_);
        auto it = userConnMap_.find(toid);
        if (it != userConnMap_.end()) {
            //toid在线，转发消息 服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }

    //toid不在线，存储离线消息
    offlineMsgModel_.insert(toid, js.dump());
}

// 添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    //存储好友信息
    friendModel_.insert(userid, friendid);
}

//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    //存储新创建的群组消息
    Group group(-1, name, desc);
    if (groupModel_.createGroup(group)) {
        //存储群组创建人信息
        groupModel_.addGroup(userid, group.getId(), "creator");
    }
}
//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    groupModel_.addGroup(userid, groupid, "normal");
}
//群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = groupModel_.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(connMutex_);
    for (int id : useridVec) {
        auto it = userConnMap_.find(id);
        if (it != userConnMap_.end()) {
            //转发群消息
            it->second->send(js.dump());
        } else {
            //查询toid是否在线
            User user = userModel_.query(id); 
            offlineMsgModel_.insert(id, js.dump());
        }
    }

}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn) {
    User user;
    {
        lock_guard<mutex> lock(connMutex_);
        for (auto it = userConnMap_.begin(); it != userConnMap_.end(); ++it) {
            if (it->second == conn) {
                // 从map表删除用户的链接信息
                user.setId(it->first);
                userConnMap_.erase(it);
                break;
            }
        }
    }

    // // 用户注销，相当于就是下线，在redis中取消订阅通道
    // _redis.unsubscribe(user.getId()); 

    // 更新用户的状态信息
    if (user.getId() != -1) {
        user.setState("offline");
        userModel_.updateState(user);
    }
}