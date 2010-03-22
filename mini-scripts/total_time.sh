#!/bin/bash
grep "Total Time:" | awk '{print $3}'