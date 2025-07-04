savedcmd_/home/veda/factory/feeder/feeder_driver.mod := printf '%s\n'   feeder_driver.o | awk '!x[$$0]++ { print("/home/veda/factory/feeder/"$$0) }' > /home/veda/factory/feeder/feeder_driver.mod
