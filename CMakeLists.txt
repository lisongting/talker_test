cmake_minimum_required(VERSION 2.8)

project(talker)

set(SOURCE  FileUtil.cpp
                main.cpp
                Talker.cpp
                linuxrec.cpp
                speech_recognizer.cpp
            )
		

#aux_source_directory(. SOURCE)

include_directories(include)

add_executable(talker  ${SOURCE})

target_link_libraries(talker   msc pthread  aiui asound )
