#!/bin/bash
username=`whoami`
database=pet_project_tests_db
password=pet_project_password
method=md5
sudo -i -u postgres psql --quiet <<EOF
SELECT 'CREATE ROLE $username WITH LOGIN PASSWORD ''$password'' '
WHERE NOT EXISTS(SELECT FROM pg_catalog.pg_roles WHERE pg_roles.rolname = '$username')\gexec
SELECT 'CREATE DATABASE $database OWNER $username'
WHERE NOT EXISTS(SELECT FROM pg_database WHERE datname = '$database')\gexec
EOF
psql --quiet -U $username -d $database -f pet_project_tests_db_tables.sql >/dev/null
