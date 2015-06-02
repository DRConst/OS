#Makefile for CloudShell Project


all: Accounting MissionControl Client

Accounting: AccountingSrc/Accounting.c 
	gcc -o Accounting AccountingSrc/Accounting.c
			
MissionControl: MissionControlSrc/MissionControl.c
	gcc -lcrypt -o MissionControl MissionControlSrc/MissionControl.c
			
Client:
	gcc -o Client ClientSrc/Client.c
