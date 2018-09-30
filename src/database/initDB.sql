CREATE TABLE users (
    username TEXT,
    pwSalt TEXT,
    pwHash TEXT,
    join_date TEXT,
    last_login TEXT,
    online_now INTEGER,
    PRIMARY KEY(username)
);

CREATE TABLE messages (
    time TEXT,
    message TEXT,
    sent_by TEXT,
    PRIMARY KEY(time),
    FOREIGN KEY(sent_by) REFERENCES users(username)
);

/* admin (pw: 'admin') hashed with sha256(pw + salt) */
INSERT INTO users VALUES ("admin", "IAmSalty", "B63D242A14099FED208090E5FFB9956180418ACF0D0651F837E5F75FF27F66EC", datetime('now'), NULL, 0);
/* user (pw: '1234') hashed with sha256(pw + salt) */
INSERT INTO users VALUES ("user", "128376512376512357621364", "9F7CF64AD85D21DD570D2361ACDC6453D47E18B2905A6C4F46442F2A21FF6432", datetime('now'), NULL, 0);

INSERT INTO messages VALUES (datetime('now'), "Welcome!", "admin");
