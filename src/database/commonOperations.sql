-- show messages, who send them, and if they are online
SELECT message, username, online_now FROM messages JOIN users WHERE sent_by = username;
-- change admin user to offline
UPDATE users SET online_now = 0 WHERE username = 'admin';
-- count online users
SELECT count(*) FROM users WHERE online_now = 1;