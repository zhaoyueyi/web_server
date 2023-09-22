// webserver
create database webserver;

// 创建user表
USE webserver;
CREATE TABLE user(
                     username char(50) NULL,
                     password char(50) NULL
)ENGINE=InnoDB;

// 添加数据
INSERT INTO user(username, password) VALUES('name', 'password');