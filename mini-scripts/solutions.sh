#!/bin/bash
grep "Solutions Found:" | awk '{print $3}'