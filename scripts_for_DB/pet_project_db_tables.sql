CREATE SEQUENCE song_seq;

CREATE TABLE song_table (
	id bigint DEFAULT NEXTVAL('song_seq'),
	song_name varchar NOT NULL,
	song_uid varchar  NOT NULL,
	song_URL varchar NOT NULL
);

CREATE SEQUENCE log_seq;

CREATE TABLE log_table (
	id bigint DEFAULT NEXTVAL('log_seq'),
        requested_host varchar NOT NULL,
	port varchar NOT NULL,
	ip inet NOT NULL,
        user_agent varchar NOT NULL,
	REST_method varchar NOT NULL,
	target varchar NOT NULL,
	req_date_time timestamp (0) without time zone NOT NULL	
);
