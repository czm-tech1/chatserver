-- sql脚本的执行命令 source /home/cherry/aTopWork/chatserver/thirdparty/tables.sql
-- 创建 user 表
CREATE TABLE user (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(50) NOT NULL UNIQUE,
    password VARCHAR(50) NOT NULL,
    state ENUM('online', 'offline') DEFAULT 'offline'
);

-- 创建 friend 表
CREATE TABLE friend (
    userid INT NOT NULL,
    friendid INT NOT NULL,
    PRIMARY KEY (userid, friendid)
);

-- 创建 allgroup 表
CREATE TABLE allgroup (
    id INT PRIMARY KEY AUTO_INCREMENT,
    groupname VARCHAR(50) NOT NULL UNIQUE,
    groupdesc VARCHAR(200) DEFAULT ''
);

-- 创建 groupuser 表
CREATE TABLE groupuser (
    groupid INT NOT NULL,
    userid INT NOT NULL,
    grouprole ENUM('creator', 'normal') DEFAULT 'normal',
    PRIMARY KEY (groupid, userid)
);

-- 创建 offlinemessage 表
CREATE TABLE offlinemessage (
    userid INT NOT NULL,
    message VARCHAR(500) NOT NULL
);

-- 关于offlinemessage表，我们应该允许某一个userid能够存储多条离线信息，userid上线后进行转发
-- 确保移除旧的主键约束（如果存在）
ALTER TABLE offlinemessage
DROP PRIMARY KEY;

-- 添加新的自增列 id 并设为主键
ALTER TABLE offlinemessage
ADD COLUMN id INT AUTO_INCREMENT PRIMARY KEY FIRST;

-- 确保添加新的主键约束和 userid 列的索引
ALTER TABLE offlinemessage
ADD INDEX (userid);
