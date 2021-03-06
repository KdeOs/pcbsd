#include "mainUI.h"
#include "ui_mainUI.h"

mainUI::mainUI(QWidget *parent) : QMainWindow(parent), ui(new Ui::mainUI){
  //Initialize the graphical items
  ui->setupUi(this);  //load the mainUI.ui file
  revMenu = new QMenu();
  brMenu = new QMenu();
  addMenu = new QMenu();
  //Setup the menu system
  ui->tool_revert->setMenu(revMenu);
  ui->tool_browse->setMenu(brMenu);
  ui->tool_add->setMenu(addMenu);
  connect(revMenu,SIGNAL(triggered(QAction*)),this,SLOT(slotRevertToSnapshot(QAction*)) );
  connect(brMenu,SIGNAL(triggered(QAction*)),this,SLOT(slotBrowseSnapshot(QAction*)) );
  connect(addMenu, SIGNAL(triggered(QAction*)),this,SLOT(slotAddDataset(QAction*)) );
  //Setup the Key menu items (static items, never changed)
  keyMenu = new QMenu();
    keyMenu->addAction(ui->actionKeyNew); //action from designer
    keyMenu->addAction(ui->actionKeyCopy); //action from designer
  ui->tool_keys->setMenu(keyMenu);
  //Setup the update frequency limiter
  freqTimer = new QTimer();
	freqTimer->setSingleShot(true);
	freqTimer->setInterval(15000);
	connect(freqTimer, SIGNAL(timeout()), this, SLOT(setupUI()) );
}

mainUI::~mainUI(){
	
}

void mainUI::setupUI(){
  //Initialize the Hash (make sure it is not run too frequently - causes kernel panics)
  if(lastUpdate.isNull() || lastUpdate.addSecs(15) < QTime::currentTime() ){
    lastUpdate = QTime::currentTime();	  
    qDebug() << "Updating the database";
    updateHash();
  }else{
    freqTimer->start();
  }
  //Update the display
  updateUI();
  updateMenus();
}

LPDataset mainUI::newDataset(QString ds){
  //subroutine to create and fill a new dataset with system information
  qDebug() << "New Dataset: " << ds;
  LPDataset DSC;
  //List all the mountpoints in this dataset
  QStringList subsets = LPBackend::listDatasetSubsets(ds);
  //populate the list of snapshots available for each mountpoint
  for(int i=0; i<subsets.length(); i++){
    //qDebug() << "Subset:" << subsets[i];
    QStringList snaps = LPBackend::listSnapshots(subsets[i]);
    //qDebug() << " - Snapshots:" << snaps;
    if(snaps.isEmpty()){
      //invalid subset - remove it from the list
      subsets.removeAt(i);
      i--;
    }else{
      DSC.subsetHash.insert(subsets[i],snaps); //add it to the internal container hash
    }
  }
  //Get the time for the latest life-preserver snapshot (and total number)
  //Find the index for the current list
  int ci = 0;
  while(ci < CLIST.length()){
    if(CLIST[ci].startsWith(ds+":::")){ break; }
    else{ ci++; }
  }
  if(CLIST.isEmpty()){ ci = -1; } //catch for empty list
  if(DSC.subsetHash.size() < 1){
    DSC.numberOfSnapshots = "0";
    DSC.latestSnapshot= "";
  }else{
    QStringList fSnap = DSC.subsetHash[subsets[0]].filter("auto-"); //filtered snapshot list (just life preserver snapshots)
    DSC.numberOfSnapshots = QString::number(fSnap.length());
    if(fSnap.isEmpty()){ DSC.latestSnapshot=""; }
    else if(ci > -1 && ci < CLIST.length()){ 
      QString sna = CLIST[ci].section(":::",1,1);
      if(sna != "-"){ DSC.latestSnapshot= sna; }
      else{ DSC.latestSnapshot = ""; }      
    }else{ DSC.latestSnapshot=fSnap[0]; }
  }
  //List the replication status
  if(RLIST.contains(ds) && (ci > -1)){ 
    QString rep = CLIST[ci].section(":::",2,2);
    if(rep != "-"){ DSC.latestReplication = rep; }
    else{ DSC.latestReplication= tr("Enabled"); }
  }else{ 
    DSC.latestReplication= tr("Disabled");
  }
  //Return the dataset
  return DSC;
}

// =================
//    PRIVATE FUNCTIONS
// =================
void mainUI::updateHash(QString ds){
  //qDebug() << "Get replication targets";
  RLIST = LPBackend::listReplicationTargets(); //update list of replication datasets
  //qDebug() << "Get possible datasets";
  SLIST = LPBackend::listPossibleDatasets();
  //qDebug() << "List current status";
  CLIST = LPBackend::listCurrentStatus();
  //qDebug() << "Check hash";
  if(HLIST.contains(ds) && !ds.isEmpty()){
    //only update the entry for the given dataset
    HLIST.insert(ds, newDataset(ds)); //will overwrite the current entry in the hash
  }else{
    //Clear and fill the hash
    //qDebug() << "Clear hash";
    HLIST.clear();
    //qDebug() << "List datasets";
    QStringList dsList = LPBackend::listDatasets();
    for(int i=0; i<dsList.length(); i++){
      HLIST.insert( dsList[i], newDataset(dsList[i]) );
    }
  }
  //qDebug() << "Done with Hash Update";
}

void mainUI::updateUI(){
  ui->treeWidget->clear();
  QStringList dsList = HLIST.keys();
  for(int i=0; i<dsList.length(); i++){
    //Create the item for the treeWidget
    // [ dataset, latest snapshot, num snapshots, is Replicated ]
    QStringList cols;
	  cols << dsList[i]; //[0] - dataset
	  cols << HLIST[dsList[i]].latestSnapshot; // [1] newest snapshot name
	  cols << HLIST[dsList[i]].numberOfSnapshots; // [2] total number of snapshots
	  cols << HLIST[dsList[i]].latestReplication; // [3] latest replication
    //Add the item to the widget
    ui->treeWidget->addTopLevelItem( new QTreeWidgetItem(cols) );
  }
  //Now adjust the columns in the widget
  for(int i=0; i<4; i++){
    ui->treeWidget->resizeColumnToContents(i);
  }
  //Now make sure that the add button menu only shows the available datasets
  addMenu->clear();
  for(int i=0; i<SLIST.length(); i++){
    if(!HLIST.contains(SLIST[i])){ addMenu->addAction( new QAction(SLIST[i],this) ); }
  }
  if(addMenu->isEmpty()){ ui->tool_add->setEnabled(false); }
  else{ ui->tool_add->setEnabled(true); }
}

void mainUI::updateMenus(){
  //Reset the button menu's to correspond to the selected dataset
  QString ds = getSelectedDS();
  //Enable/disable the remove/config buttons if nothing selected
  if(ds.isEmpty()){
    ui->tool_remove->setVisible(false);
    ui->tool_config->setVisible(false);

  }else{
    ui->tool_remove->setVisible(true);
    ui->tool_config->setVisible(true);	  
  }
  //Enabled/disable the SSH key management
  if(RLIST.contains(ds) && !ds.isEmpty()){
    ui->tool_keys->setVisible(true);
  }else{
    ui->tool_keys->setVisible(false);
  }
  //check for a valid ds/snapshot combination
  bool ok = !ds.isEmpty();
  if(ok){ ok = HLIST.contains(ds); }
  if(ok){ ok = (HLIST[ds].numberOfSnapshots.toInt() > 0); }
  //Now set the items appropriately
  revMenu->clear();
  brMenu->clear();
  if(ok){
    //Reset the Menu Contents
    QStringList subsets = HLIST[ds].subsets();	
    for(int i=0; i<subsets.length(); i++){
      //Build the menu of snapshots for this subset
	QStringList snaps = HLIST[ds].snapshots(subsets[i]);
	if(snaps.isEmpty()){ continue; }
	QMenu *menu = new QMenu(subsets[i],this);
	for(int s =0; s<snaps.length(); s++){
	  QAction *act = new QAction(snaps[s],this);
		act->setWhatsThis(ds+":::"+subsets[i]+":::"+snaps[s]);
	  menu->addAction(act);
	}
	revMenu->addMenu(menu);
	brMenu->addMenu(menu);
    }	    
    //Enable the buttons if appropriate
    if(revMenu->isEmpty()){
      ui->tool_revert->setEnabled(false);
      ui->tool_browse->setEnabled(false);
    }else{
      ui->tool_revert->setEnabled(true);
      ui->tool_browse->setEnabled(true);
    }
  }else{
    //Disable the buttons
    ui->tool_revert->setEnabled(false);
    ui->tool_browse->setEnabled(false);
  }
}

QString mainUI::getSelectedDS(){
  //Get the currently selected dataset
  QString ds="";
  if(ui->treeWidget->topLevelItemCount() > 0){
    if( ui->treeWidget->currentItem() != 0){
      ds = ui->treeWidget->currentItem()->text(0); //first column
    }
  }
  return ds;
}

// =================
//        PRIVATE SLOTS
// =================
//  --- Buttons Clicked
void mainUI::on_treeWidget_itemSelectionChanged(){
  updateMenus();
}

void mainUI::on_tool_config_clicked(){
  QString ds = getSelectedDS();
  if(ds.isEmpty()){ return; }
  LPConfig CFG(this);
  CFG.loadDataset(ds, RLIST.contains(ds));
  CFG.exec();
  //Now check for return values and update appropriately
  bool change = false;
  if(CFG.localChanged){
    LPBackend::setupDataset(ds, CFG.localSchedule, CFG.localSnapshots);
    change = true;
  }
  if(CFG.remoteChanged){
    change = true;
    if(CFG.isReplicated){
      LPBackend::setupReplication(ds, CFG.remoteHost, CFG.remoteUser, CFG.remotePort, CFG.remoteDataset, CFG.remoteFreq);
      QMessageBox::information(this,tr("Reminder"),tr("Don't forget to save your SSH key to a USB stick so that you can restore your system from the remote host later!!"));
    }else{
      LPBackend::removeReplication(ds);
    }
  }
  //Now update the UI if appropriate
  if(change){
    setupUI();
  }
}

void mainUI::on_tool_remove_clicked(){
  QString ds = getSelectedDS();
  if(!ds.isEmpty()){
    //Verify the removal of the dataset
    if( QMessageBox::Yes == QMessageBox::question(this,tr("Verify Dataset Backup Removal"),tr("Are you sure that you wish to cancel automated snapshots and/or replication of the following dataset?")+"\n\n"+ds,QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ){	    
      //verify the removal of all the snapshots for this dataset
      QStringList snaps = LPBackend::listLPSnapshots(ds);
      if(!snaps.isEmpty()){
        if( QMessageBox::Yes == QMessageBox::question(this,tr("Verify Snapshot Deletion"),tr("Do you wish to remove the local snapshots for this dataset?")+"\n"+tr("WARNING: This is a permanant change that cannot be reversed"),QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ){
	  //Remove all the snapshots
	  for(int i=0; i<snaps.length(); i++){
	    LPBackend::removeSnapshot(ds,snaps[i]);
	  }
        }
      }
      //Remove the dataset from life-preserver management
      if(RLIST.contains(ds)){ LPBackend::removeReplication(ds); }
      LPBackend::removeDataset(ds);
    }
  }
  setupUI();
}


// --- Menu Items Clicked
void mainUI::slotRevertToSnapshot(QAction *act){
  QString info = act->whatsThis();
  QString ds = info.section(":::",0,0);
  QString subset = info.section(":::",1,1);
  QString snap = info.section(":::",2,2);
  qDebug() << "Revert Clicked:" << ds << subset << snap;
  if(!ds.isEmpty()){
    //Verify the reversion 
     if( QMessageBox::Yes == QMessageBox::question(this,tr("Verify Snapshot Reversion"),
	     QString(tr("Are you sure that you wish to revert %1 to the selected snapshot?")).arg(subset)+"\n"+tr("WARNING: This will result in the loss of any data not previously backed up."),
	     QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ){
        //Perform the reversion
        if( !LPBackend::revertSnapshot(ds+subset,snap) ){
	  //Error performing the reversion
	  qDebug() << " - Error:" << ds+subset << snap;
	  QMessageBox::warning(this,tr("Reversion Error"), tr("The snapshot reversion could not be completed successfully."));
	}else{
	  //Good reversion
	  qDebug() << " - Revert Complete";
	  QMessageBox::information(this,tr("Reversion Success"), tr("The snapshot reversion was completed successfully."));	
	}
     }
  }
}

void mainUI::slotBrowseSnapshot(QAction *act){
  QString info = act->whatsThis();
  QString ds = info.section(":::",0,0);
  QString subset = info.section(":::",1,1);
  QString snap = info.section(":::",2,2);
  //Now let the user select a file within the snapshot to revert
  QString snapPath = subset+"/.zfs/snapshot/"+snap+"/";
  QString filepath = QFileDialog::getOpenFileName(this,tr("Revert a file"), snapPath, tr("Backup Files (*)") );
  qDebug() << "File to revert:" << filepath;
  //Check to make sure that it is a valid file (within the snapshot)
  if(filepath.isEmpty() ){
    qDebug() << " - Cancelled";
    //action cancelled  -  do nothing
  }else if(!filepath.startsWith(snapPath)){
    qDebug() << " - Invalid File";
    QMessageBox::warning(this, tr("Invalid Snapshot File"), tr("Please select a file from within the chosen snapshot that you wish to revert"));
  }else{
    //Revert the file
    QString newfile = LPBackend::revertSnapshotFile(subset,snap,filepath);
    if(newfile.isEmpty()){
      //Error copying the new file over
      qDebug() << " - Error copying file";
      QMessageBox::warning(this, tr("Error Reverting File"), QString(tr("An error occurred while tring to revert the file %1. Please try again.")).arg(filepath));
    }else{
      //Let the user know the location of the reverted file
      qDebug() << " - Successful reversion:" << newfile;
      QMessageBox::information(this, tr("FIle Reverted"), QString(tr("The reverted file is now available at: %1")).arg(newfile) );
    }
  }
  return;
}

void mainUI::slotAddDataset(QAction *act){
  QString dataset = act->text();
  qDebug() << "Start Wizard for new dataset:" << dataset;
  LPWizard wiz(this);
  wiz.setDataset(dataset);
  wiz.exec();
  //See if the wizard was cancelled or not
  if(!wiz.cancelled){
    //run the proper commands to get the dataset enabled
    if( LPBackend::setupDataset(dataset, wiz.localTime, wiz.totalSnapshots) ){
      if(wiz.enableReplication){
	 LPBackend::setupReplication(dataset, wiz.remoteHost, wiz.remoteUser, wiz.remotePort, wiz.remoteDataset, wiz.remoteTime);     
	 QMessageBox::information(this,tr("Reminder"),tr("Don't forget to save your SSH key to a USB stick so that you can restore your system from the remote host later!!"));
      }
    }
  }
  //Now update the UI/Hash
  setupUI();
}

void mainUI::on_actionClose_triggered(){
  this->close();
}

void mainUI::on_actionKeyNew_triggered(){
  QString ds = getSelectedDS();
  qDebug() << "New SSH Key triggered for DS:" << ds;
  //Get the remote values for this dataset
  QString remoteHost, user, remotedataset;
  int port, time;
  bool ok = LPBackend::replicationInfo(ds, remoteHost, user, port, remotedataset,  time);
  if(ok){
    if( !LPBackend::setupSSHKey(remoteHost, user, port) ){
      QMessageBox::warning(this,tr("Failure"), tr("There was an error while creating the SSH key."));
    }else{
      QMessageBox::information(this,tr("Success"), tr("The SSH key was successfully generated."));
    }
  }else{
    QMessageBox::warning(this,tr("Failure"), tr("There was an error in retrieving the remote replication information for this dataset. Please ensure that replication is enabled and try agin.") );
  }
}

void mainUI::on_actionKeyCopy_triggered(){
  QString ds = getSelectedDS();	
  qDebug() << "Copy SSH Key triggered for DS:" << ds;
  //Get the local hostname
  char host[1023] = "\0";
  gethostname(host,1023);
  QString localHost = QString(host).simplified();
  qDebug() << " - hostname:" << localHost;
  //Scan for mounted USB devices
  QStringList devs = LPBackend::findValidUSBDevices();
  qDebug() << " - devs:" << devs;
  if(devs.isEmpty()){
    QMessageBox::warning(this,tr("No Valid USB Devices"), tr("No valid USB devices could be found. Please mount a FAT32 formatted USB stick and try again."));
    return;
  }
  //Ask the user which one to save the file to
  bool ok;
  QString dev = QInputDialog::getItem(this, tr("Select USB Device"), tr("Available USB Devices:"), devs,0,false,&ok);	
  if(!ok or dev.isEmpty()){ return; } //cancelled
  QString devPath = dev.section("(",0,0).simplified();
  //Now copy the file over
  ok = LPBackend::copySSHKey(devPath, localHost);
  if(ok){
    QMessageBox::information(this,tr("Success"), tr("The public SSH key file was successfully copied onto the USB device."));
  }else{
    QMessageBox::information(this,tr("Failure"), tr("The public SSH key file could not be copied onto the USB device."));
  }
}


// =============
//      PROTECTED
// =============
void mainUI::closeEvent(QCloseEvent *event){
  //Make sure this window only gets hidden rather than closed
  // this prevents the entire tray application from closing down as well
  event->ignore();
  this->hide();
}
