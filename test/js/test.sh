#!/bin/bash

# Make sure the script runs in the directory in which it is placed
cd $(dirname `[[ $0 = /* ]] && echo "$0" || echo "$PWD/${0#./}"`)
if [ -z $TARGET_OS ]; then
    target_os=local
else
    target_os=$TARGET_OS
fi
echo "PWD: $(pwd)"
executable="../../dist/pro/$target_os/bin/vrpc-agent-pro"
plugin="../../dist/pro/$target_os/lib/test_plugin.so"

$executable -d test.vrpc -a cppTestAgent -p $plugin -t $VRPC_TEST_TOKEN -b ssl://81.169.211.210:8883 & agent_pid=$!
sleep 2
../../node_modules/.bin/mocha . --exit & mocha_pid=$!
EXIT_CODE=$?
sleep 20
kill -9 "$agent_pid" > /dev/null 2>&1
kill -9 "$mocha_pid" > /dev/null 2>&1
exit $EXIT_CODE
