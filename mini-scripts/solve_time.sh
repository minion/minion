#!/bin/bash
grep "Solve Time:" | awk '{print $3}'