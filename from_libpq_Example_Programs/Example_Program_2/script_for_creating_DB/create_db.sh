#!/bin/bash
username=`whoami`
database=testlibpq
#sudo -i -u postgres psql -f postgres_role_db.sql
sudo -i -u postgres psql <<EOF
SELECT 'CREATE ROLE $username  WITH LOGIN'
WHERE NOT EXISTS(SELECT FROM pg_catalog.pg_roles WHERE pg_roles.rolname = '$username')\gexec
SELECT 'CREATE DATABASE $database OWNER  $username'
WHERE NOT EXISTS(SELECT FROM pg_database WHERE datname = '$database')\gexec
EOF
