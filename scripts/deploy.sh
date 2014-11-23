#!/bin/bash

PEM="$HOME/gnome.pem"
EC2="ubuntu@ec2-54-85-142-145.compute-1.amazonaws.com"

make html
scp -i $PEM -r build $EC2:~/www
