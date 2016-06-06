#!/bin/bash
#Installation script for additions for pilight 7
#

red=`tput setaf 1`
green=`tput setaf 2`
yellow=`tput setaf 3`
reset=`tput sgr0`

DOWNLOAD_FOLDER="pilight-addons"

PROTOCOL_DESTINATION_FOLDER="/usr/local/lib/pilight/protocols"
PROTOCOL_LIBS_FOLDER="libs/pilight/protocols"

ACTION_DESTINATION_FOLDER="/usr/local/lib/pilight/actions"
ACTION_LIBS_FOLDER="libs/pilight/events/actions"

OPERATOR_DESTINATION_FOLDER="/usr/local/lib/pilight/operators"
OPERATOR_LIBS_FOLDER="libs/pilight/events/operators"

FUNCTION_DESTINATION_FOLDER="/usr/local/lib/pilight/functions"
FUNCTION_LIBS_FOLDER="libs/pilight/events/functions"

GITHUB_PATH="https://github.com/niekd/${DOWNLOAD_FOLDER}.git" 

clear

cd /home/pi/pilight || { echo "${red}pilight directory not found, manually install pilight development version first!${reset}";  exit 1; }
echo "Installation script for pilight addons"
echo
echo "You are logged in as user: $USER"
echo "Current directory is: $PWD"
echo

# Stop pilight service
echo "${green}Stop the pilight service....${reset}"
/etc/init.d/pilight stop || { echo "${red}Stopping pilight service failed!${reset}"; exit 1; }

# Download source code
echo "${green}Download source code....${reset}"
if [ -d "${DOWNLOAD_FOLDER}" ]; then
	read -p "${yellow}Download directory (${DOWNLOAD_FOLDER}) exists, remove it [y/n]?${reset}" -n 1 -r
	echo    
	if [[ $REPLY =~ ^[Yy]$ ]]; then
			rm -R ${DOWNLOAD_FOLDER}
	else
		echo "${red}Aborting${reset}"
		echo
		exit 1;	
	fi
fi
git clone -b master ${GITHUB_PATH} || { echo "${red}Download failed!${reset}"; exit 1; }

echo "${green}Install additional protocol(s):${reset}"
echo
echo "- webswitch"
echo "- wunderful"
echo "- generic_counter"
echo
read -p "${yellow}Do you want to install these additional protocol(s)  [y/n]?${reset}" -n 1 -r
echo    
if [[ $REPLY =~ ^[Yy]$ ]]; then

	mkdir -p ${PROTOCOL_LIBS_FOLDER}  || { echo "${red}Creation of ${PROTOCOL_LIBS_FOLDER} failed!${reset}"; exit 1; }
	cp ${DOWNLOAD_FOLDER}/protocols/*.* ${PROTOCOL_LIBS_FOLDER}/API || { echo "${red}Copying protocol source files failed!${reset}"; exit 1; }

	# Compile protocol modules
	echo "${green}Compile the protocol modules....${reset}"
	gcc -fPIC -shared ${PROTOCOL_LIBS_FOLDER}/API/webswitch.c -Ilibs/pilight/protocols/ -Iinc/ -o webswitch.so -DMODULE=1
	gcc -fPIC -shared ${PROTOCOL_LIBS_FOLDER}/API/wunderful.c -Ilibs/pilight/protocols/ -Iinc/ -o wunderful.so -DMODULE=1
	gcc -fPIC -shared ${PROTOCOL_LIBS_FOLDER}/generic/generic_counter.c -Ilibs/pilight/protocols/ -Iinc/ -o generic_counter.so -DMODULE=1


	# Copy the protocol modules to destination
	echo "${green}Copy the protocolmodules to their destination....${reset}"
	mkdir -p ${PROTOCOL_DESTINATION_FOLDER} || { echo "${red}Creation of ${PROTOCOL_DESTINATION_FOLDER} failed!${reset}"; exit 1; }
	cp webswitch.so wugstation.so generic_counter.so${PROTOCOL_DESTINATION_FOLDER} || { echo "${red}Copying to destination ${PROTOCOL_DESTINATION_FOLDER} failed!${reset}"; exit 1; }

fi

echo "${green}Install additional functions(s):${reset}"
echo
echo "- LOOKUP"
echo
read -p "${yellow}Do you want to install these additional functions(s)  [y/n]?${reset}" -n 1 -r
echo    
if [[ $REPLY =~ ^[Yy]$ ]]; then

	mkdir -p ${FUNCTION_LIBS_FOLDER}  || { echo "${red}Creation of ${FUNCTION_LIBS_FOLDER} failed!${reset}"; exit 1; }
	cp ${DOWNLOAD_FOLDER}/functions/*.* ${FUNCTION_LIBS_FOLDER} || { echo "${red}Copying protocol source files failed!${reset}"; exit 1; }

	# Compile function modules
	echo "${green}Compile the function module(s)....${reset}"
	gcc -fPIC -shared ${FUNCTION_LIBS_FOLDER}/lookup.c -Ilibs/pilight/events/ -Iinc/ -o lookup.so -DMODULE=1

	# Copy the function modules to destination
	echo "${green}Copy the function module(s) to their destination....${reset}"
	mkdir -p ${FUNCTION_DESTINATION_FOLDER} || { echo "${red}Creation of ${FUNCTION_DESTINATION_FOLDER} failed!${reset}"; exit 1; }
	cp lookup.so ${FUNCTION_DESTINATION_FOLDER} || { echo "${red}Copying to destination ${FUNCTION_DESTINATION_FOLDER} failed!${reset}"; exit 1; }

fi

echo "${green}Install additional action(s):${reset}"
echo
echo "- count"
echo "- file"
echo
read -p "${yellow}Do you want to install these additional actions(s)  [y/n]?${reset}" -n 1 -r
echo    
if [[ $REPLY =~ ^[Yy]$ ]]; then

	mkdir -p ${ACTION_LIBS_FOLDER}  || { echo "${red}Creation of ${ACTION_LIBS_FOLDER} failed!${reset}"; exit 1; }
	cp ${DOWNLOAD_FOLDER}/actions/*.* ${ACTION_LIBS_FOLDER} || { echo "${red}Copying protocol source files failed!${reset}"; exit 1; }

	# Compile action modules
	echo "${green}Compile the action module(s)....${reset}"
	gcc -fPIC -shared ${ACTION_LIBS_FOLDER}/count.c -Ilibs/pilight/events/ -Iinc/ -o count.so -DMODULE=1
	gcc -fPIC -shared ${ACTION_LIBS_FOLDER}/file.c -Ilibs/pilight/events/ -Iinc/ -o file.so -DMODULE=1

	# Copy the action modules to destination
	echo "${green}Copy the action module(s) to their destination....${reset}"
	mkdir -p ${ACTION_DESTINATION_FOLDER} || { echo "${red}Creation of ${ACTION_DESTINATION_FOLDER} failed!${reset}"; exit 1; }
	cp count.so file.so ${ACTION_DESTINATION_FOLDER} || { echo "${red}Copying to destination ${ACTION_DESTINATION_FOLDER} failed!${reset}"; exit 1; }

fi

# Delete download folder
read -p "${yellow}Download directory ${DOWNLOAD_FOLDER} is no longer needed, delete it now [y/n]?${reset}" -n 1 -r
echo    
if [[ $REPLY =~ ^[Yy]$ ]]; then
	rm -R ${DOWNLOAD_FOLDER} || { echo "${red}Deletion of ${DOWNLOAD_FOLDER} failed!${reset}"; exit 1; }
fi

# Restart pilight
echo "${green}Restart pilight....${reset}"
/etc/init.d/pilight start || { echo "${red}Starting pilight service failed!${reset}"; exit 1; }

echo
echo "Installation completed without errors"
echo

