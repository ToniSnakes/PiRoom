CREATE TABLE users (username varchar(255), password varchar(255), join_date text, last_login text, online_now integer);
CREATE TABLE messages (message varchar(1000), time text, sent_by varchar(255));

INSERT INTO users VALUES ("admin", "admin", datetime('now'), datetime('now'), 0);
INSERT INTO users VALUES ("user", "1234", datetime('now'), datetime('now'), 0);

INSERT INTO messages VALUES ("Welcome!", datetime('now'), "admin");
