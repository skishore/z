#!/bin/bash

coffee -o static/javascript/ -c coffee
sass --update scss:static/css
