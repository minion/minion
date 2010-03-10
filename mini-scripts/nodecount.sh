#!/bin/bash
grep "Total Nodes:" | awk '{print $3}'