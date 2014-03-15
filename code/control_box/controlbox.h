/**************************** controlbox.h *****************************

Code to manage the primary user interface to include the QDialog the
user interfaces with and the system tray icon.  

Copyright (C) 2013-2014
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

# ifndef CONTROLBOX_H
# define CONTROLBOX_H

# include <QtDBus/QtDBus>
# include <QtDBus/QDBusInterface>
# include <QtDBus/QDBusMessage>
# include <QtDBus/QDBusObjectPath>
# include <QDialog>
# include <QString>
# include <QMap>
# include <QVariant>
# include <QSystemTrayIcon>
# include <QAction>
# include <QTimer>
# include <QActionGroup>
# include <QCommandLineParser>

# include "ui_controlbox.h"
# include "./code/agent/agent.h"
# include "./code/counter/counter.h"

//	Used for enum's local to this program
namespace CMST 
{
  enum {
    // errors
    No_Errors	= 0x00,
    Err_No_DBus	= 0x01, 			// Can't find DBus 
    Err_Invalid_Iface	= 0x02, // Invalid interface
    Err_Properties	= 0x04,		// There was an error reading connman.Manager.GetProperties
    Err_Technologies= 0x08,		// There was an error reading connman.Manager.GetTechnologies       
    Err_Services		= 0x10,		// There was an error reading connman.Manager.GetServices  
  };
} // namespace CMST


//	Two of the connman.Manager query functions will return an array of structures.
//	This struct provides a receiving element we can use to collect the return data.
struct arrayElement
{
	QDBusObjectPath objpath;
	QMap<QString,QVariant> objmap;
};


//	The main program class based on a QDialog
class ControlBox : public QDialog
{
  Q_OBJECT

  public:
    ControlBox(const QCommandLineParser&, QWidget* parent = 0);

  public slots:  
		void aboutCMST();
		void aboutNuvola();
		void showLicense();
		void showChangeLog();
		
  private:
  // members 
    Ui::ControlBox ui;
		quint8 q8_errors;
		QMap<QString,QVariant>	properties_map;
		QList<arrayElement>		services_list;
		QList<arrayElement>		technologies_list;
		QList<arrayElement>		wifi_list;
		ConnmanAgent* agent;
		ConnmanCounter* counter;		
		QDBusObjectPath service_online;
		short wifi_interval;		
		quint32 counter_accuracy;	
		quint32 counter_period;				
		QDBusInterface* iface_manager;
		QTimer* wifi_timer;
		QSystemTrayIcon*	trayicon; 
		QActionGroup* minMaxGroup;
		QAction* minimizeAction;
		QAction* maximizeAction;
		QAction* exitAction;
	
	// functions
		void	updateDisplayWidgets();
		void assemblePage1();
		void assemblePage2();
		void assemblePage3();
		void assemblePage4();
		void assembleTrayIcon();
		bool getProperties();
		bool getTechnologies();		
		bool getServices();
		bool getArray(QList<arrayElement>&, const QDBusMessage&);
		bool getMap(QMap<QString,QVariant>&, const QDBusMessage&); 
		bool extractMapData(QMap<QString,QVariant>&,const QVariant&);
		void logErrors(const quint8&);
		QString readResourceText(const char*);  

  private slots:
		void counterUpdated(const QDBusObjectPath&, const QString&, const QString&);
		void connectPressed();
		void disconnectPressed();
		void removePressed();
		void propertyChanged();
		void scanWifi();
		void toggleOfflineMode(bool);
		void toggleTrayIcon(bool);
		void togglePowered(int, int);
		void minMaxWindow(QAction* = 0);
		void getServiceDetails(int);
		void showWhatsThis();
	
};

#endif

