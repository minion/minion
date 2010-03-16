#!/bin/bash
grep "Sol:" | awk '{$1 = ""; print }'