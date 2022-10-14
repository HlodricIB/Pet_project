CREATE TABLE song_table (
	id bigserial PRIMARY KEY,
	song_name varchar NOT NULL,
	song_uid varchar  NOT NULL,
	song_URL varchar NOT NULL
);

CREATE TABLE log_table (
	id bigserial PRIMARY KEY,
	host varchar NOT NULL,
	port serial NOT NULL,
	ip inet NOT NULL,
	REST_method varchar NOT NULL,
	target varchar NOT NULL,
	req_date_time timestamp (0) with time zone NOT NULL	

);
