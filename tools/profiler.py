#!/usr/bin/python

import xml.dom.minidom
import sys

class ProfilerOutput:
	"""A profiler output file"""

	COLS = [35, 10, 15, 15]

	filename = ""
	xmlf = None
	entries = []

	def __init__(self, filename):
		self.filename = filename
		self.parse(self.filename)
		
	def parse(self, filename):
		self.xmlf = xml.dom.minidom.parse(filename)

	def analyze(self):
		self.analyzeRic(self.xmlf)

	def analyzeRic(self, xmlf):		

		for e in xmlf.childNodes:
			if e.nodeType == e.ELEMENT_NODE:
			
				entry = ProfilerEntry()

				entry.name = e.localName
				entry.caller = e.getAttribute("caller")
				entry.timesCalled = e.getAttribute("called")
				entry.avrTime = e.getAttribute("avr-time")
				entry.totTime = e.getAttribute("tot-time")

				self.entries.append(entry)

				self.analyzeRic(e)

	def formatTime(self, stime):

		time = float(stime)

		if time < 1000:
			return str(round(time,2))+" ms"
		elif time >= 1000 and time < (1000*60):
			return str(round(time/1000,2))+" s"
		elif time >= (1000*60):
			mn = int(time/(1000*60))
			sec = int((time - (mn*1000*60))/1000)
			return str(mn)+":"+str(sec)+" s"


	def showFunction(self, name, indent = 0):

		for e in self.entries:
			if e.caller == name:

				print (((indent+1)*".")+e.name).ljust(self.COLS[0]), e.timesCalled.ljust(self.COLS[1]),
				print self.formatTime(e.avrTime).ljust(self.COLS[2]),self.formatTime(e.totTime).ljust(self.COLS[3])

				self.showFunction(e.name, indent+1)

	def show(self):

		print "Function name".ljust(self.COLS[0]),"Called".ljust(self.COLS[1]),
		print "Avg. Time".ljust(self.COLS[2]), "Tot. Time".ljust(self.COLS[3])

		print "-"*self.COLS[0],"-"*self.COLS[1],"-"*self.COLS[2],"-"*self.COLS[3]

		fr = self.entries[0]

		print fr.name.ljust(self.COLS[0]), fr.timesCalled.ljust(self.COLS[1]),
		print self.formatTime(fr.avrTime).ljust(self.COLS[2]),self.formatTime(fr.totTime).ljust(self.COLS[3])

		self.showFunction(fr.name)


	def order(self):
		self.entries.sort(key=lambda x: x.totTime, reverse=True)

				
				

class ProfilerEntry:
	"""An entry in a profiler xml output"""
	
	name = ""
	caller = ""
	timesCalled = 0
	avrTime = 0
	totTime = 0

if len(sys.argv) < 2: 
	print "Please specify a filename"
	exit()

filename = sys.argv[1]

print "Analyzing file: ",filename

profout = ProfilerOutput(filename)
profout.analyze()
profout.show()

