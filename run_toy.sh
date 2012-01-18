#!/bin/bash

# A top-level script to guide the process of building the
# toy WFST cascade. Run _after_ the training is completed,
# (i.e. start after './run.sh' is done)  

mkdir -p data_toy
steps/prepare_toy_graphs.sh
