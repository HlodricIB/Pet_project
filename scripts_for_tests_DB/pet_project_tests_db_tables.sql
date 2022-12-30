SET client_min_messages TO WARNING;

CREATE SEQUENCE IF NOT EXISTS song_seq START 1;

CREATE TABLE IF NOT EXISTS song_table (
	id bigint DEFAULT NEXTVAL('song_seq'),
	song_name varchar NOT NULL,
	song_uid varchar  NOT NULL,
	song_URL varchar NOT NULL
);

CREATE SEQUENCE IF NOT EXISTS log_seq START 1;

CREATE TABLE IF NOT EXISTS log_table (
	id bigint DEFAULT NEXTVAL('log_seq'),
        requested_host varchar NOT NULL,
	port varchar NOT NULL,
	ip inet NOT NULL,
        user_agent varchar NOT NULL,
	REST_method varchar NOT NULL,
	target varchar NOT NULL,
	req_date_time timestamp (0) without time zone NOT NULL	
);

TRUNCATE song_table RESTART IDENTITY;
TRUNCATE log_table RESTART IDENTITY;

SELECT setval('song_seq', 1);
SELECT setval('log_seq', 1);

INSERT INTO song_table (id, song_name, song_uid, song_URL) VALUES (currval('song_seq'), 'song1.wav', 11111, '/some_dir/1');
INSERT INTO song_table (id, song_name, song_uid, song_URL) VALUES (nextval('song_seq'), 'song2.mp3', 11112, '/some_dir/2');
INSERT INTO song_table (id, song_name, song_uid, song_URL) VALUES (nextval('song_seq'), 'song3.aac', 11113, '/some_dir/3');
INSERT INTO song_table (id, song_name, song_uid, song_URL) VALUES (nextval('song_seq'), 'song4', 11114, '/some_dir/4');
INSERT INTO log_table (id, requested_host, port, ip, user_agent, rest_method, target, req_date_time) VALUES (currval('log_seq'),
                        '127.0.0.41:8080', 11111, '127.0.0.1', 'tests_client_1', 'METHOD_1', 'target_1', TIMESTAMP '2022-12-21 10:15:20');
INSERT INTO log_table (id, requested_host, port, ip, user_agent, rest_method, target, req_date_time) VALUES (nextval('log_seq'),
                        '127.0.0.42:8080', 11112, '127.0.0.2', 'tests_client_2', 'METHOD_2', 'target_2', TIMESTAMP '2022-12-21 12:15:20');
INSERT INTO log_table (id, requested_host, port, ip, user_agent, rest_method, target, req_date_time) VALUES (nextval('log_seq'),
                        '127.0.0.43:8080', 11113, '127.0.0.3', 'tests_client_3', 'METHOD_3', 'target_3', TIMESTAMP '2022-12-21 13:15:20');
INSERT INTO log_table (id, requested_host, port, ip, user_agent, rest_method, target, req_date_time) VALUES (nextval('log_seq'),
                        '127.0.0.44:8080', 11114, '127.0.0.4', 'tests_client_4', 'METHOD_4', 'target_4', TIMESTAMP '2022-12-23 14:15:20');
