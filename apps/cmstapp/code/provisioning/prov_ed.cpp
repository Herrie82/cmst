/****************** prov_ed.cpp ***********************************

Code to manage the Provisioning Editor dialog.

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
# include <QRegularExpressionValidator>
# include <QDBusMessage>
# include <QDBusConnection>
# include <QDBusInterface>
# include <QMessageBox>
# include <QInputDialog>
# include <QList>
# include <QVariant>
# include <QAction>
# include <QFile>
# include <QFileDialog>

# include "./prov_ed.h"
# include "../resource.h"

ValidatingDialog::ValidatingDialog(QWidget* parent) : QDialog(parent)
{
  // build the dialog
  label = new QLabel(this);
  lineedit = new QLineEdit(this);
  buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  this->setSizeGripEnabled(true);
  
  QVBoxLayout* vboxlayout = new QVBoxLayout;
  vboxlayout->addWidget(label);
  vboxlayout->addWidget(lineedit);
  vboxlayout->addWidget(buttonbox);
  this->setLayout(vboxlayout);

  // signals and slots
  connect(buttonbox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonbox, SIGNAL(rejected()), this, SLOT(reject()));
}

// Slot to set the lineedit validator
void ValidatingDialog::setValidator(const int& vd, bool plural)
{
  // setup a switch to set the validator
  QString s_ip4   = "(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])";
  QString s_ip6   = "(?:[0-9a-fA-F]{1,4})";
  QString s_mac   = "(?:[0-9a-fA-F]{1,2})";
  QString s_hex   = "[0-9a-fA-F]*";
  QString s_dom   = "[0-9a-zA-Z]*[\.]?[0-9a-zA-Z]*";
  QString s_wd    = "[0-9,a-zA-Z_\.\!\@\#\$\%\^\&\*\+\-]*";
  QString s_start = (plural ? "\\s?|(" : "\\s?|^");
  QString s_end   = (plural ? "(\\s*[,|;|\\s]\\s*))+" : "$");
  
  switch (vd){
    case CMST::ProvEd_Vd_IPv4: {
      QRegularExpression rx4(s_start + s_ip4 + "(?:\." + s_ip4 + "){3}" + s_end);
      QRegularExpressionValidator* lev_4 = new QRegularExpressionValidator(rx4, this);
      lineedit->setValidator(lev_4); }
      break;
    case CMST::ProvEd_Vd_IPv6: {
      QRegularExpression rx6(s_start + s_ip6 + "(?::" + s_ip6 + "){7}" + s_end);
      QRegularExpressionValidator* lev_6 = new QRegularExpressionValidator(rx6, this);
      lineedit->setValidator(lev_6); }
      break;
    case CMST::ProvEd_Vd_MAC: {
      QRegularExpression rxm(s_start + s_mac + "(?::" + s_mac + "){5}" + s_end);
      QRegularExpressionValidator* lev_m = new QRegularExpressionValidator(rxm, this); 
      lineedit->setValidator(lev_m); }
      break;
    case CMST::ProvEd_Vd_46: {
      QRegularExpression rx46(s_start + "(" + s_ip4 + "(?:\." + s_ip4 + "){3}|" + s_ip6 + "(?::" + s_ip6 + "){7})" + s_end);    
      QRegularExpressionValidator* lev_46 = new QRegularExpressionValidator(rx46, this);  
      lineedit->setValidator(lev_46); }
      break;  
    case CMST::ProvEd_Vd_Hex: {
      QRegularExpression rxh(s_start + s_hex + s_end);
      QRegularExpressionValidator* lev_h = new QRegularExpressionValidator(rxh, this);
      lineedit->setValidator(lev_h); }
      break;
    case CMST::ProvEd_Vd_Dom: {
      QRegularExpression rxdom(s_start + s_dom + s_end);
      QRegularExpressionValidator* lev_dom = new QRegularExpressionValidator(rxdom, this);
      lineedit->setValidator(lev_dom); }
      break;
    case CMST::ProvEd_Vd_Wd: {
      QRegularExpression rxwd(s_start + s_wd + s_end);
      QRegularExpressionValidator* lev_wd = new QRegularExpressionValidator(rxwd, this);
      lineedit->setValidator(lev_wd); }
      break;        
    default:
      lineedit->setValidator(0);
      break;
    } // switch     
    
  return;
}
   

ProvisioningEditor::ProvisioningEditor(QWidget* parent) : QDialog(parent)
{
  // Setup the user interface
  ui.setupUi(this);

  // Data members
  menubar = new QMenuBar(this);
  ui.verticalLayout01->setMenuBar(menubar);

  statusbar = new QStatusBar(this);
  ui.verticalLayout01->addWidget(statusbar);
  statustimeout = 2000;
  
  filename = "";
  i_sel = CMST::ProvEd_No_Selection;
  
  // Setup the buttongroup
  bg01 = new QButtonGroup(this);
  bg01->addButton(ui.pushButton_open);
  bg01->addButton(ui.pushButton_save);
  bg01->addButton(ui.pushButton_delete);
    
  // Add actions to actiongroups (signals from actiongroups are connected to slots)
  group_template = new QActionGroup(this);
  group_template->addAction(ui.actionTemplateEduroamLong);
  group_template->addAction(ui.actionTemplateEduroamShort);
  
  group_freeform = new QActionGroup(this);
  group_freeform->addAction(ui.actionGlobal);
  group_freeform->addAction(ui.actionGlobalName);
  group_freeform->addAction(ui.actionGlobalDescription);
  group_freeform->addAction(ui.actionService);
  group_freeform->addAction(ui.actionWifiPrivateKeyPassphrase);
  group_freeform->addAction(ui.actionWifiIdentity);
  group_freeform->addAction(ui.actionWifiPassphrase); 
  group_freeform->addAction(ui.actionWifiPhase2);
  
  group_combobox = new QActionGroup(this);
  group_combobox->addAction(ui.actionServiceType);
  group_combobox->addAction(ui.actionWifiEAP);
  group_combobox->addAction(ui.actionWifiPrivateKeyPassphraseType);
  group_combobox->addAction(ui.actionWifiSecurity);
  group_combobox->addAction(ui.actionWifiHidden);
  group_combobox->addAction(ui.actionServiceIPv6Privacy);
  
  group_validated = new QActionGroup(this);
  group_validated->addAction(ui.actionServiceMAC);
  group_validated->addAction(ui.actionWifiSSID);
  group_validated->addAction(ui.actionServiceNameServers);
  group_validated->addAction(ui.actionServiceTimeServers);
  group_validated->addAction(ui.actionServiceSearchDomains);
  group_validated->addAction(ui.actionServiceDomain);
  group_validated->addAction(ui.actionWifiName);
  
  group_selectfile = new QActionGroup(this);
  group_selectfile->addAction(ui.actionWifiCACertFile);
  group_selectfile->addAction(ui.actionWifiClientCertFile);
  group_selectfile->addAction(ui.actionWifiPrivateKeyFile);
  
  group_ipv4 = new QActionGroup(this);
  group_ipv4->addAction(ui.actionServiceIPv4Off);
  group_ipv4->addAction(ui.actionServiceIPV4DHCP);
  group_ipv4->addAction(ui.actionServiceIPv4Address);

  group_ipv6 = new QActionGroup(this);
  group_ipv6->addAction(ui.actionServiceIPv6Off);
  group_ipv6->addAction(ui. actionServiceIPv6Auto);
  group_ipv6->addAction(ui.actionServiceIPv6Address);
  
  // Add Actions from UI to menu's
  menu_global = new QMenu(tr("Global"), this);
  menu_global->addAction(ui.actionGlobal);
  menu_global->addSeparator();
  menu_global->addAction(ui.actionGlobalName);
  menu_global->addAction(ui.actionGlobalDescription);
  
  menu_service = new QMenu(tr("Service"), this);
  menu_service->addAction(ui.actionService);
  menu_service->addSeparator();
  menu_service->addAction(ui.actionServiceType);
  menu_service->addAction(ui.actionServiceDomain);
  menu_service->addAction(ui.actionServiceMAC);
  menu_service->addSeparator();
  menu_service->addAction(ui.actionServiceIPv4Off);
  menu_service->addAction(ui.actionServiceIPV4DHCP);
  menu_service->addAction(ui.actionServiceIPv4Address);
  menu_service->addSeparator();
  menu_service->addAction(ui.actionServiceIPv6Off);
  menu_service->addAction(ui. actionServiceIPv6Auto);
  menu_service->addAction(ui.actionServiceIPv6Address);
  menu_service->addAction(ui.actionServiceIPv6Privacy);
  menu_service->addSeparator();
  menu_service->addAction(ui.actionServiceNameServers);
  menu_service->addAction(ui.actionServiceTimeServers);
  menu_service->addAction(ui.actionServiceSearchDomains);
  
  menu_wifi = new QMenu(tr("WiFi"), this);
  menu_wifi->addSeparator();
  menu_wifi->addAction(ui.actionWifiName);
  menu_wifi->addAction(ui.actionWifiSSID);
  menu_wifi->addSeparator();
  menu_wifi->addAction(ui.actionWifiSecurity);
  menu_wifi->addAction(ui.actionWifiPassphrase);
  menu_wifi->addAction(ui.actionWifiHidden);
  menu_wifi->addAction(ui.actionWifiPhase2);
  menu_wifi->addSeparator();
  menu_wifi->addAction(ui.actionWifiEAP);
  menu_wifi->addAction(ui.actionWifiIdentity);  
  menu_wifi->addSeparator();
  menu_wifi->addAction(ui.actionWifiCACertFile);
  menu_wifi->addAction(ui.actionWifiClientCertFile);
  menu_wifi->addSeparator();
  menu_wifi->addAction(ui.actionWifiPrivateKeyFile);
  menu_wifi->addAction(ui.actionWifiPrivateKeyPassphrase);
  menu_wifi->addAction(ui.actionWifiPrivateKeyPassphraseType);
  
  menu_template = new QMenu(tr("Templates"), this);menu_template->addAction(ui.actionTemplateEduroamShort);
  menu_template->addAction(ui.actionTemplateEduroamLong);
  menu_template->addAction(ui.actionTemplateEduroamShort);
  
  // add menus to UI
  menubar->addMenu(menu_global);
  menubar->addMenu(menu_service);
  menubar->addMenu(menu_wifi);
  menubar->addMenu(menu_template);  
  
  // connect signals to slots
  connect(ui.toolButton_whatsthis, SIGNAL(clicked()), this, SLOT(showWhatsThis()));
  connect(ui.pushButton_resetpage, SIGNAL(clicked()), this, SLOT(resetPage()));
  connect(bg01, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(requestFileList(QAbstractButton*)));
  connect(group_template, SIGNAL(triggered(QAction*)), this, SLOT(templateTriggered(QAction*)));
  connect(group_freeform, SIGNAL(triggered(QAction*)), this, SLOT(inputFreeForm(QAction*)));
  connect(group_combobox, SIGNAL(triggered(QAction*)), this, SLOT(inputComboBox(QAction*)));
  connect(group_validated, SIGNAL(triggered(QAction*)), this, SLOT(inputValidated(QAction*)));
  connect(group_selectfile, SIGNAL(triggered(QAction*)), this, SLOT(inputSelectFile(QAction*)));
  connect(group_ipv4, SIGNAL(triggered(QAction*)), this, SLOT(ipv4Triggered(QAction*)));
  connect(group_ipv6, SIGNAL(triggered(QAction*)), this, SLOT(ipv6Triggered(QAction*)));
  
  // signals from dbus
  //QDBusConnection::systemBus().connect("org.cmst.roothelper", "/", "org.cmst.roothelper", "obtainedFileList", this, SLOT(processFileList(const QStringList&)));
  //QDBusConnection::systemBus().connect("org.cmst.roothelper", "/", "org.cmst.roothelper", "fileReadCompleted", this, SLOT(seedTextEdit(const QString&)));
  //QDBusConnection::systemBus().connect("org.cmst.roothelper", "/", "org.cmst.roothelper", "fileDeleteCompleted", this, SLOT(deleteCompleted(bool)));
  //QDBusConnection::systemBus().connect("org.cmst.roothelper", "/", "org.cmst.roothelper", "fileWriteCompleted", this, SLOT(writeCompleted(quint64)));
}

/////////////////////////////////////////////// Private Slots /////////////////////////////////////////////
//
// Slot called when a member of the QActionGroup group_selectfile
void ProvisioningEditor::inputSelectFile(QAction* act)
{
  // variables
  QString key = act->text();
  QString title;
  
  if (act == ui.actionWifiCACertFile) title = tr("File Path to the CA Certificate File");
  if (act == ui.actionWifiClientCertFile) title = tr("File Path to the Client Certificate File");
  if (act == ui.actionWifiPrivateKeyFile) title = tr("File path to the Client Private Key File");;
    
  
  QString fname = QFileDialog::getOpenFileName(this, title,
                      QDir::homePath(),
                      tr("Key Files (*.pem);;All Files (*.*)"));

  // return if the file name returned is empty (cancel pressed in the dialog)
  if (fname.isEmpty() ) return;

  // put the path into the text edit
  key.append(" = %1\n");
  ui.plainTextEdit_main->insertPlainText(key.arg(fname) );
  
  return;
}


//
// Slot called when a member of the QActionGroup group_validated is triggered
void ProvisioningEditor::inputValidated(QAction* act)
{
  // variables
  QString key = act->text();
  
  // create the dialog
  ValidatingDialog* vd = new ValidatingDialog(this);
  
  // create some prompts and set validator
  if (act == ui.actionServiceMAC) {vd->setLabel(tr("MAC address.")); vd->setValidator(CMST::ProvEd_Vd_MAC);}
  if (act == ui.actionWifiSSID) {vd->setLabel(tr("SSID: hexadecimal representation of an 802.11 SSID")), vd->setValidator(CMST:: ProvEd_Vd_Hex);}
  if (act == ui.actionServiceNameServers) {vd->setLabel(tr("List of Nameservers")), vd->setValidator(CMST::ProvEd_Vd_46, true);}
  if (act == ui.actionServiceTimeServers) {vd->setLabel(tr("List of Timeservers")), vd->setValidator(CMST::ProvEd_Vd_46, true);}
  if (act == ui.actionServiceSearchDomains) {vd->setLabel(tr("List of DNS Search Domains")), vd->setValidator(CMST::ProvEd_Vd_Dom, true);}
  if (act == ui.actionServiceDomain) {vd->setLabel(tr("Domain name to be used")), vd->setValidator(CMST::ProvEd_Vd_Dom);}
  if (act == ui.actionWifiName) {vd->setLabel(tr("Enter the string representation of an 802.11 SSID.")), vd->setValidator(CMST::ProvEd_Vd_Wd);}
  
  // if accepted put an entry in the textedit
  if (vd->exec() == QDialog::Accepted) {
    QString s = vd->getText();
    key.append(" = %1\n");
    
    // format strings with multiple entries
    if (vd->isPlural() ) {
      s.replace(',', ' ');
      s.replace(';', ' ');
      s = s.simplified();
      s.replace(' ', ',');
    }
    
    ui.plainTextEdit_main->insertPlainText(key.arg(s) );
  }  
  
  // cleanup
  vd->deleteLater();
  return;
}

//
// Slot called when a member of the QActionGroup group_combobox is triggered
void ProvisioningEditor::inputComboBox(QAction* act)
{
  // variables
  QString key = act->text();
  QString str;
  bool ok;
  QStringList sl;
  
  // create some prompts
  if (act == ui.actionServiceType) {str = tr("Service type."); sl << "ethernet" << "wifi";}
  if (act == ui.actionWifiEAP) {str = tr("EAP type."); sl << "tls" << "ttls" << "peap";}
  if (act == ui.actionWifiPrivateKeyPassphraseType) {str = tr("Private key passphrase type."); sl << "fsid";}
  if (act == ui.actionWifiSecurity) {str = tr("Network security type."); sl << "psk" << "ieee8021x" << "wep" << "none";}
  if (act == ui.actionWifiHidden) {str = tr("Hidden network"); sl << "true" << "false";}
  if (act == ui.actionServiceIPv6Privacy) {str = tr("IPv6 Privacy"); sl << "disabled" << "enabled" << "preferred";}
  
  QString item = QInputDialog::getItem(this,
    tr("%1 - Item Input").arg(PROGRAM_NAME),
    str,
    sl,
    0,
    false,
    &ok);
    
  key.append(" = %1\n");
  if (ok) ui.plainTextEdit_main->insertPlainText(key.arg(item));
  
  return;
}
//
// Slot called when a member of the QActionGroup group_freeform is triggered
// Freeform strings may have spaces in them.  For strings that cannot have spaces
// use validated text and set b_multiple to false.
void ProvisioningEditor::inputFreeForm(QAction* act)
{
  // variables
  const QLineEdit::EchoMode echomode = QLineEdit::Normal;
  QString str;
  bool ok;  
  QString key = act->text();
  
  // create some prompts
  if (act == ui.actionService) str = tr("Tag which will replace the * with<br>an identifier unique to the config file.");
  if (act == ui.actionGlobalName) str = tr("Enter the network name.");
  if (act == ui.actionGlobalDescription)  str = tr("Enter a description of the network.");
  if (act == ui.actionWifiPrivateKeyPassphrase) str = tr("Password/Passphrase for the private key file.");
  if (act == ui.actionWifiIdentity) str = tr("Identity string for EAP.");
  if (act == ui.actionWifiPassphrase) str = tr("RSN/WPA/WPA2 Passphrase");
  if (act == ui.actionWifiPhase2) str = tr("Phase 2 (inner authentication with TLS tunnel)<br>authentication method.");   
  
  if (act == ui.actionGlobal) {
    key.append("\n");
    ui.plainTextEdit_main->insertPlainText(key);
  }
  else {
    act == ui.actionService ? key = "[service_%1]\n" : key.append(" = %1\n"); 
    
    // get the string from the user
    QString text = "";
      text = QInputDialog::getText(this,
        tr("%1 - Text Input").arg(PROGRAM_NAME),
        str,
        echomode,
        "",
        &ok);
  
    if (ok) ui.plainTextEdit_main->insertPlainText(key.arg(text));
  } // else   
  
  return;
}

//
//  Slot called when a member of the QActionGroup group_ipv4 is triggered
void ProvisioningEditor::ipv4Triggered(QAction* act)
{
  // variables
  QString s = "IPv4 = %1\n";
  QString val;

  // process action
  if (act == ui.actionServiceIPv4Off) ui.plainTextEdit_main->insertPlainText(s.arg("off") );
  if (act == ui.actionServiceIPV4DHCP) ui.plainTextEdit_main->insertPlainText(s.arg("dhcp") );
  if (act == ui.actionServiceIPv4Address) {
    QMessageBox::StandardButton but = QMessageBox::information(this, 
                                        QString(PROGRAM_NAME) + tr("- Information"),
                                        tr("The IPv4 <b>Address</b>, <b>Netmask</b>, and optionally <b>Gateway</b> need to be provided."  \
                                        "<p>Press OK when you are ready to proceed."),
                                        QMessageBox::Ok | QMessageBox::Abort,QMessageBox::Ok);
    if (but == QMessageBox::Ok) {
      ValidatingDialog* vd = new ValidatingDialog(this);
      vd->setLabel(tr("IPv4 Address"));
      vd->setValidator(CMST::ProvEd_Vd_IPv4);
      if (vd->exec() == QDialog::Accepted && ! vd->getText().isEmpty() ) {
        val = vd->getText();
        vd->clear();
        vd->setLabel(tr("IPv4 Netmask")); 
        vd->setValidator(CMST::ProvEd_Vd_IPv4);
        if (vd->exec() == QDialog::Accepted && ! vd->getText().isEmpty() ) {
          val.append("/" + vd->getText() );
          vd->clear();
          vd->setLabel(tr("IPv4 Gateway (This is an optional entry)")); 
          vd->setValidator(CMST::ProvEd_Vd_IPv4);
          if (vd->exec() == QDialog::Accepted && ! vd->getText().isEmpty() ) { 
            val.append("/" + vd->getText() );
          } // if gateway accpted
          ui.plainTextEdit_main->insertPlainText(s.arg(val) );
        } // if netmask accepted
      } // if address accepted 
      vd->deleteLater();
    } // we pressed OK on the information dialog
  } // act == actionServiceIPv4Address
  
  return;
}

//
// Slot called when a member of the QActonGroup group_ipv6 is triggered
void ProvisioningEditor::ipv6Triggered(QAction* act)
{
  // variables
  QString s = "IPv6 = %1\n";
  bool ok;
  QString val;

  // process action
  if (act == ui.actionServiceIPv6Off) ui.plainTextEdit_main->insertPlainText(s.arg("off") );
  if (act == ui.actionServiceIPv6Auto) ui.plainTextEdit_main->insertPlainText(s.arg("auto") );
  if (act == ui.actionServiceIPv6Address) {
    QMessageBox::StandardButton but = QMessageBox::information(this, 
                                        QString(PROGRAM_NAME) + tr("- Information"),
                                        tr("The IPv6 <b>Address</b>, <b>Prefix Length</b>, and optionally <b>Gateway</b> need to be provided."  \
                                        "<p>Press OK when you are ready to proceed."),
                                        QMessageBox::Ok | QMessageBox::Abort,QMessageBox::Ok);
    if (but == QMessageBox::Ok) {
      ValidatingDialog* vd = new ValidatingDialog(this);
      vd->setLabel(tr("IPv6 Address"));
      vd->setValidator(CMST::ProvEd_Vd_IPv6);
      if (vd->exec() == QDialog::Accepted && ! vd->getText().isEmpty() ) {
        val = vd->getText();
        int i = QInputDialog::getInt(this,
          tr("%1 - Integer Input").arg(PROGRAM_NAME),
          tr("Enter the IPv6 prefix length"),
          0, 0, 255, 1,
          &ok);
        if (ok) {
          val.append(QString("/%1").arg(i) );
          ValidatingDialog* vd = new ValidatingDialog(this);
          vd->setLabel(tr("IPv6 Gateway (This is an optional entry)")); 
          vd->setValidator(CMST::ProvEd_Vd_IPv6);
          if (vd->exec() == QDialog::Accepted && ! vd->getText().isEmpty() ) {
            val.append(QString("/" + vd->getText()) );
          } // if gateway was accepted
          ui.plainTextEdit_main->insertPlainText(s.arg(val) );
        } // if prefix provided 
      } // if address accepted
      vd->deleteLater();
    } // we pressed OK on the informaion dialog
  } // act == actionServiceIPv6Address
  
  return;
}

//
// Slot called when a member of the QActionGroup group_template is triggered
void ProvisioningEditor::templateTriggered(QAction* act)
{
  // variable
  QString source;
  
  // get the source string depending on the action
  if (act == ui.actionTemplateEduroamLong) source = ":/text/text/eduroam_long.txt";
  else if (act == ui.actionTemplateEduroamShort) source = ":/text/text/eduroam_short.txt";
  
  // get the text
  QFile file(source);
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {  
    QByteArray ba = file.readAll();
    
    // seed the textedit with the template
    this->seedTextEdit(QString(ba));
  } // if 
  

  return;
}

//
//  Slot to enter whats this mode
//  Called when the ui.toolButton_whatsthis clicked() signal is emitted
void ProvisioningEditor::showWhatsThis()
{
  QWhatsThis::enterWhatsThisMode();
}

//
// Function to clear the contents of the textedit
void ProvisioningEditor::resetPage()
{
  ui.plainTextEdit_main->document()->clear();

  return;
}

//
// Slot to request a file list from the roothelper.
// Roothelper will emit an obtainedFileList signal when finished.  This slot 
// is connected to the QButtonGroup bg01
void ProvisioningEditor::requestFileList(QAbstractButton* button)
{
  // initialize the selection
  if (button == ui.pushButton_open) i_sel = CMST::ProvEd_File_Read;
  else if (button == ui.pushButton_save) i_sel = CMST::ProvEd_File_Write;
    else if (button == ui.pushButton_delete) i_sel = CMST::ProvEd_File_Delete;
      else i_sel = CMST::ProvEd_No_Selection;
  
  // request a list of config files from roothelper
  QList<QVariant> vlist;
  QDBusInterface* iface_rfl = new QDBusInterface("org.cmst.roothelper", "/", "org.cmst.roothelper", QDBusConnection::systemBus(), this);
  iface_rfl->callWithCallback(QLatin1String("getFileList"), vlist, this, SLOT(processFileList(const QStringList&)), SLOT(callbackErrorHandler(QDBusError)));
  
  iface_rfl->deleteLater();  
  return;
}

//
// Slot to process the file list from /var/lib/connman.  Connected to
// the obtainedFileList signal in roothelper
void ProvisioningEditor::processFileList(const QStringList& sl_conf)
{
  // variables
  bool ok;
  filename.clear();
  QList<QVariant> vlist;
  QDBusInterface* iface_pfl = new QDBusInterface("org.cmst.roothelper", "/", "org.cmst.roothelper", QDBusConnection::systemBus(), this);
  
  // If we are trying to open and read the file
  if (i_sel & CMST::ProvEd_File_Read) {
    // display dialogs based on the length of the stringlist
    switch (sl_conf.size()) {
      case 0:
        QMessageBox::information(this, 
          QString(PROGRAM_NAME) + tr("- Information"),
          tr("<center>No configuration files were found.<br>You may use this dialog to create one."),
          QMessageBox::Ok,
          QMessageBox::Ok);
        break; 
      case 1:
        QMessageBox::information(this,
          tr("%1 - Information").arg(PROGRAM_NAME),
          tr("<center>Reading configuration file: %1").arg(sl_conf.at(0)),
          QMessageBox::Ok,
          QMessageBox::Ok);
        filename = sl_conf.at(0);
        break;
      default:
        QString item = QInputDialog::getItem(this,
            tr("%1 - Select File").arg(PROGRAM_NAME),
            tr("Select a file to load."),
            sl_conf,
            0,
            false,
            &ok);
        if (ok) filename = item;    
        break;
      } // switch 
    // if we have a filename try to open the file
    if (! filename.isEmpty() ) {
      vlist.clear();
      vlist << QVariant::fromValue(filename); 
      iface_pfl->callWithCallback(QLatin1String("readFile"), vlist, this, SLOT(seedTextEdit(const QString&)), SLOT(callbackErrorHandler(QDBusError)));    
    } // if there is a file name
  } // if i_sel is File_Read
  
  // If we are trying to delete the file
  else if (i_sel & CMST::ProvEd_File_Delete) {
    // // user will have to select the file to delete it
    switch (sl_conf.size()) {     
      case 0:
        QMessageBox::information(this, 
          QString(PROGRAM_NAME) + tr("- Information"),
          tr("<center>No configuration files were found.<br>Nothing will be deleted."),
          QMessageBox::Ok,
          QMessageBox::Ok);
        break; 
      default:
        QString item = QInputDialog::getItem(this,
            tr("%1 - Select File").arg(PROGRAM_NAME),
            tr("Select a file to be deleted."),
            sl_conf,
            0,
            false,
            &ok);
        if (ok) filename = item;    
        break;
      } // switch
    // if we have a filename try to delete the file
    if (! filename.isEmpty() ) {
      vlist.clear();
      vlist << QVariant::fromValue(filename);
      iface_pfl->callWithCallback(QLatin1String("deleteFile"), vlist, this, SLOT(deleteCompleted(bool)), SLOT(callbackErrorHandler(QDBusError)));    
    } // if there is a file name
  } // if i_sel is File_Delete      
  
  // If we are trying to save the file
  else if (i_sel & CMST::ProvEd_File_Write) {
  QString item = QInputDialog::getItem(this,
      tr("%1 - Select File").arg(PROGRAM_NAME),
      tr("Enter a new file name or select<br>an existing file to overwrite."),
      sl_conf,
      0,
      true,
      &ok);
    if (ok) filename = item;    
    // if we have a filename try to save the file
    if (! filename.isEmpty() ) {
      vlist.clear();
      vlist << QVariant::fromValue(filename);
      vlist << QVariant::fromValue(ui.plainTextEdit_main->toPlainText() );
      iface_pfl->callWithCallback(QLatin1String("saveFile"), vlist, this, SLOT(writeCompleted(quint64)), SLOT(callbackErrorHandler(QDBusError)));   
    } // if there is a file name
  } // if i_sel is File_Save  
      
  // cleanup
  i_sel = CMST::ProvEd_No_Selection;
  iface_pfl->deleteLater();   
  return;
}

//
// Slot to seed the QTextEdit window with data read from file.  Connected to
// fileReadCompleted signal in root helper.  Also called directly from
// the templateTriggered slot. 
void ProvisioningEditor::seedTextEdit(const QString& data)
{
  // clear the text edit and seed it with the read data
  ui.plainTextEdit_main->document()->clear();
  ui.plainTextEdit_main->setPlainText(data);
  
  // show a statusbar message
  statusbar->showMessage(tr("File read completed"), statustimeout);
  
  return;
}

//
// Slot to show a statusbar message when a file delete is completed
//void ProvisioningEditor::deleteCompleted(bool success)
void ProvisioningEditor::deleteCompleted(bool success)
{
  QString msg;
  
  if (success)
    msg = tr("File deleted");
  else
    msg = tr("Error encountered deleting.");
  
  statusbar->showMessage(msg, statustimeout);
  return;
} 

//
// Slot to show a statusbar message when a file write is completed
void ProvisioningEditor::writeCompleted(quint64 bytes)
{
  // display a status bar message showing the results of the write
  QString msg;
  
  if (bytes < 0 )
    msg = tr("File save failed.");
  else {
    if (bytes > 1024)
      msg = tr("%L1 KB written").arg(bytes / 1024);
    else  
      msg = tr("%L1 Bytes written").arg(bytes);
  }
  
  statusbar -> showMessage(msg, statustimeout);
  return;
}

//
// Slot to handle errors from callWithCallback functions
void ProvisioningEditor::callbackErrorHandler(QDBusError err)
{
	QMessageBox::critical(this,
		QString(PROGRAM_NAME) + tr("- Critical"),
		QString(tr("<b>DBus Error Name:</b> %1<br><br><b>String:</b> %2<br><br><b>Message:</b> %3")).arg(err.name()).arg(err.errorString(err.type())).arg(err.message()),
		QMessageBox::Ok,
		QMessageBox::Ok);
		
	return;
}
