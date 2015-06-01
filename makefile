#Makefile for CloudShell Project


all : Accounting MissionControl Client

Accounting : Accounting.c 
			gcc -o Accounting Accounting.c
			
MissionControl : MissionControl.c
			gcc -lcrypt -o MissionControl MissionControl.c
			
Client : Client.c
			gcc -o Client Client.c
