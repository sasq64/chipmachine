OBJDIR=obj/
TARGET=libnetlink
EXT := .a
CC = gcc
MODULES := netlink/src
CFLAGS := -Inetlink/include

include Makefile.inc