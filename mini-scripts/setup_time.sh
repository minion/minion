#!/bin/bash
grep "Setup Time:" | awk '{print $3}'