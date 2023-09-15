#!/bin/sh

cp ./.env.example ./.env
cd ./ifra

docker compose pull