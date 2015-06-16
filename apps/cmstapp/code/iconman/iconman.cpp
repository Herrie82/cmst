/**************************** iconman.cpp ******************************

Class to manage icons and allow the user to provide substitutions based
on the the system theme. 

Copyright (C) 2015
by: Andrew J. Bibb
License: MIT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
***********************************************************************/

# include "./iconman.h"
# include "../resource.h"

# include <QDir>
# include <QFile>
# include <QFileInfo>
# include <QTextStream>
# include <QDebug>

// Constructor
IconManager::IconManager(QObject* parent) : QObject(parent) 
{
	// Set the cfg member (path to ${home}/.config/cmst
	// APP defined in resource.h
	cfg = QString(qPrintable(QDir::homePath().append(QString("/.config/%1/%1.icon").arg(QString(APP).toLower()))) );	
	
	// Set the qrc data member
	qrc = QString(qPrintable(QString(":/text/text/icon_def.txt")) );
	
	// Initialize icon_map
	icon_map.clear();
	
	// Make the local conf file if necessary
	this->makeLocalFile();	
	
	// Create the icon_ map.   
	QFile f1(cfg);
	if (!f1.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug() << "Error opening icon_def file: " << cfg ;
	}
			
	QTextStream in(&f1);
	QString line;
	while (!in.atEnd()) {
		line = in.readLine();
		line = line.simplified();
		if (line.startsWith("[icon]", Qt::CaseInsensitive) ) {
			IconElement ie;
			QString iconame;
			do {
				line = in.readLine();
				if (line.startsWith("icon_name", Qt::CaseInsensitive) ) iconame = extractValue(line);
					else if (line.startsWith("resource", Qt::CaseInsensitive) ) ie.resource_path = extractValue(line);
						else if (line.startsWith("fdo_name", Qt::CaseInsensitive) ) ie.fdo_name = extractValue(line);
							else if (! line.isEmpty() ) ie.theme_map[extractKey(line)] = extractValue(line);
			} while ( ! line.isEmpty() );
		
			icon_map[iconame] = ie;
		}	// if [icon]
	}	// while not atEnd()
	f1.close();
	
	return;
}

////////////////////////////// Public Functions ////////////////////////////
//
// Function to return a QIcon based on the name provided
QIcon IconManager::getIcon(const QString& name)
{
	// Data members
	IconElement ie = icon_map.value(name);
	QMap<QString,QString> themes = ie.theme_map;
	
	// If the internal theme is being used (and the user has not
	// messed up the local config file) use that first.
	if (QIcon::themeName() == INTERNAL_THEME) {
		if (! ie.resource_path.isEmpty() ) {
			if (QFileInfo(ie.resource_path).exists() )
				return QIcon(ie.resource_path);
		}	// if resource_path notEmpty
	}	// if using internal theme
	
	// Next look for a user specified theme icon
	if (themes.contains(QIcon::themeName() )) {
		if (QIcon::hasThemeIcon(themes.value(QIcon::themeName())) )
			return QIcon::fromTheme(themes.value(QIcon::themeName()) );
	}	// if contains themeName
	
	// Next look for a freedesktop.org named icon
	if (! ie.fdo_name.isEmpty() ) {
		if (QIcon::hasThemeIcon(ie.fdo_name) )
			return QIcon::fromTheme(ie.fdo_name);
	}	// if freedesktop name not empty
			
	// Then look for hardcoded name in the users config dir
	if (! ie.resource_path.isEmpty() ) {
		if (QFileInfo(ie.resource_path).exists() )
			return QIcon(ie.resource_path);
	}	// if resource_path notEmpty	
	
	// Last stop is our fallback hard coded into the program
	return QIcon(getFallback(name));
}

////////////////////////////// Private Functions ////////////////////////////
//
// Function to return the resource name of an icon. Read from the resource file
// and only used in case the user has totally messed up his local copy of the
// cmst.icon file
QString IconManager::getFallback(const QString& ico)
{
	// Variables
	QString rtnstr = QString();
	
	// Open the resource file for reading
	QFile f0(qrc);	
	if (!f0.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug() << "Error opening resource file: " << qrc ;
		return rtnstr;
	}
	
	// Look for icon in the resource file and extract the resource value
	QTextStream in(&f0);
	QString line;
	while (!in.atEnd()) {
		line = in.readLine();
		line = line.simplified();
		if (line.startsWith("[icon]", Qt::CaseInsensitive) ) {
			QString key = "";
			QString val = "";
			do {
				line = in.readLine();
				if (line.startsWith("icon_name", Qt::CaseInsensitive) )	key = extractKey(line);
				else if (line.startsWith("resource", Qt::CaseInsensitive) )	val = extractValue(line);
			} while ( key.isEmpty() || val.isEmpty() );
				
			if (key.toLower() == ico.toLower()) {
				rtnstr = val;
				break;
			}	// key = ico
		}	// if [icon]
	}	// while not atEnd()
	f0.close();	
	
	return rtnstr;
}

//
// Function to make a local version of the configuration fiqle
void IconManager::makeLocalFile()
{
	// if the conf file exists return now
	if (QFileInfo::exists(cfg) )
		return;
		
		
	// make the directory if it does not exist and copy the hardconded
	// conf file to the directory
	QDir d;
	if (d.mkpath(QFileInfo(cfg).path()) ) {
		QFile s(qrc);	
		if (s.copy(cfg) ) 
			QFile::setPermissions(cfg, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
		else	
			qDebug() << "Failed copying the icon definition file from " << qrc << "to " << cfg;
	}	// if mkpath returned ture			
  
	return;
}

//
// Function to extract the value in the data line.  The value is the part
// after the = sign, with all white space and comments removed.  Argument
// sv is the entire line containing the value.
QString IconManager::extractValue(const QString& sv)
{
	QString s = sv.section('=', 1, 1);
	s = s.section("#", 0, 0);
	
	return s.simplified();
}

//
// Function to extract the key in the data line.  The key is the part
// before the = sign, with all white space removed.  Argument sk is the 
// entire line containing the key
QString IconManager::extractKey(const QString& sk)
{
	QString s = sk.section('=', 0, 0);
	
	return s.simplified();
}


