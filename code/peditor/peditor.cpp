
/****************** peditor.cpp *********************************** 

Code to manage the Properties Editor dialog.

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

# include <QtCore/QDebug>
# include <QRegularExpression>

# include "./code/peditor/peditor.h"

PropertiesEditor::PropertiesEditor(QWidget* parent, const QMap<QString,QVariant>& objmap, bool (*extractMapData) (QMap<QString,QVariant>&,const QVariant&))
    : QDialog(parent)
{
	// Setup the user interface
  ui.setupUi(this);
  
  // Setup the address validator and apply it to any ui QLineEdit. We'll
  // need to run entries in the QPlainTextEdit boxes separately when we process them.
  // The lev validator will validate an IP address or any amount of white space (to allow 
  // editing of the line edit).
  //QString octet = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
  QString octet = "(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])";
  QRegularExpression rx("\\s*|^" + octet + "\\." + octet + "\\." + octet + "\\." + octet + "$");
  QRegularExpressionValidator* lev = new QRegularExpressionValidator(rx, this);
  
  ui.lineEdit_ipv4address->setValidator(lev);
  ui.lineEdit_ipv4netmask->setValidator(lev);
  ui.lineEdit_ipv4gateway->setValidator(lev);
  ui.lineEdit_ipv6address->setValidator(lev);
  ui.lineEdit_ipv6gateway->setValidator(lev);
  
  // We don't want the whitespace check for things other than the QLineEdits
  rx.setPattern("^" + octet + "\\." + octet + "\\." + octet + "\\." + octet + "$");
	addressvalidator = new QRegularExpressionValidator(rx, this);
   	  
	// define and populate submaps
	QMap<QString,QVariant> ipv4map;	
	QMap<QString,QVariant> ipv6map;
	QMap<QString,QVariant> proxmap;
	extractMapData(ipv4map, objmap.value("IPv4.Configuration") );
	extractMapData(ipv6map, objmap.value("IPv6.Configuration") );
	extractMapData(proxmap, objmap.value("Proxy.Configuration") );
	
	// Seed initial values in the dialog.
	ui.plainTextEdit_nameservers->setPlainText(objmap.value("Nameservers.Configuration").toStringList().join("\n") );
	ui.plainTextEdit_timeservers->setPlainText(objmap.value("Timeservers.Congiguration").toStringList().join("\n"));
	ui.plainTextEdit_domains->setPlainText(objmap.value("Domains.configuration").toStringList().join("\n"));
	
	// ipv4 page
	if (! ipv4map.value("Method").toString().isEmpty() ) {
		ui.comboBox_ipv4method->setCurrentIndex(ui.comboBox_ipv4method->findText(ipv4map.value("Method").toString(), Qt::MatchFixedString) );
	}
	ui.lineEdit_ipv4address->setText(ipv4map.value("Address").toString() );
	ui.lineEdit_ipv4netmask->setText(ipv4map.value("Netmask").toString() );
	ui.lineEdit_ipv4gateway->setText(ipv4map.value("Gateway").toString() );
	
	// ipv6 page
	if (! ipv6map.value("Method").toString().isEmpty() ) {
		ui.comboBox_ipv6method->setCurrentIndex(ui.comboBox_ipv6method->findText(ipv6map.value("Method").toString(), Qt::MatchFixedString) );
	}
	ui.spinBox_ipv6prefixlength->setValue(ipv6map.value("PrefixLength").toInt() );
	ui.lineEdit_ipv4address->setText(ipv6map.value("Address").toString() );
	ui.lineEdit_ipv4gateway->setText(ipv6map.value("Gateway").toString() );
	if (! ipv6map.value("Privacy").toString().isEmpty() ) {
		ui.comboBox_ipv6privacy->setCurrentIndex(ui.comboBox_ipv6privacy->findText(ipv6map.value("Privacy").toString(), Qt::MatchFixedString) );
	}
	
	// proxy page
	if (! proxmap.value("Method").toString().isEmpty() ) {
		ui.comboBox_proxymethod->setCurrentIndex(ui.comboBox_proxymethod->findText(proxmap.value("Method").toString(), Qt::MatchFixedString) );
	}
	ui.plainTextEdit_proxyservers->setPlainText(proxmap.value("Servers").toStringList().join("<br>") );
	ui.plainTextEdit_proxyexcludes->setPlainText(proxmap.value("Excludes").toStringList().join("<br>") );
	ui.lineEdit_proxyurl->setText(proxmap.value("URL").toString() );
	
	
  // connect signals to slots
  connect(ui.toolButton_whatsthis, SIGNAL(clicked()), this, SLOT(showWhatsThis()));  
	connect(ui.pushButton_resetpage, SIGNAL(clicked()), this, SLOT(resetPage()));
	connect(ui.pushButton_resetall, SIGNAL(clicked()), this, SLOT(resetAll()));
  		  
}    

///////////////////////////////////////////////// Private Functions /////////////////////////////////////////////


///////////////////////////////////////////////// Private Slots /////////////////////////////////////////////
//
//  Slot to enter whats this mode
//	Called when the ui.toolButton_whatsthis clicked() signal is emitted
void PropertiesEditor::showWhatsThis()
{
  QWhatsThis::enterWhatsThisMode();
}

//
// Function to clear the contents of the specified page.  If page is
// less than one then clear the current page
void PropertiesEditor::resetPage(int page)
{
	// find the page (index) to clear.  
	int toolboxindex = ui.toolBox_peditor->currentIndex();
	if (page >= 0 ) toolboxindex = page; 
	
	switch (toolboxindex) {
		case 0:
			ui.plainTextEdit_nameservers->clear();
			break;
		case 1:
			ui.plainTextEdit_timeservers->clear();
			break;
		case 2:
			ui.plainTextEdit_domains->clear();
			break;
		case 3:
			ui.comboBox_ipv4method->setCurrentIndex(0);
			ui.lineEdit_ipv4address->clear();
			ui.lineEdit_ipv4netmask->clear();
			ui.lineEdit_ipv4gateway->clear();
			break;
		case 4:
			ui.comboBox_ipv6method->setCurrentIndex(0);
			ui.spinBox_ipv6prefixlength->setValue(0);
			ui.lineEdit_ipv6address->clear();
			ui.lineEdit_ipv6gateway->clear();
			ui.comboBox_ipv6privacy->setCurrentIndex(0);
			break;
		case 5:
			ui.comboBox_proxymethod->setCurrentIndex(0);
			ui.lineEdit_proxyurl->clear();
			ui.plainTextEdit_proxyservers->clear();
			ui.plainTextEdit_proxyexcludes->clear();
			break;
		default:
			break;
	}	// switch
		
	return;
}

//
// Slot to reset all pages
void PropertiesEditor::resetAll()
{
	for (int i = 0; i < ui.toolBox_peditor->count(); ++i) {
		this->resetPage(i);
	}
	
	return;
}

