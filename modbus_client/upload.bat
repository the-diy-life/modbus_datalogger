::"C:\Users\Peter Magdy\AppData\Local\Arduino15\packages\esp8266\tools\python\3.7.2-post1/python" "C:\Users\Peter Magdy\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.5.2/tools/upload.py" --chip esp8266 --port COM3 --baud 115200 --trace version --end --chip esp8266 --port COM3 --baud 115200 erase_flash --end  
"C:\Users\Peter Magdy\AppData\Local\Arduino15\packages\esp8266\tools\python\3.7.2-post1/python" "C:\Users\Peter Magdy\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.5.2/tools/upload.py" --chip esp8266 --port COM3 --baud 115200 --trace version --end --chip esp8266 --port COM3 --baud 115200 write_flash 0x0 modbus_client.ino.bin --end 