/*===========================================================================
This file contains the HTML served by the webserver that is minified and converted to a string for compliation. See Limit-Switch-Actuator-ControlTemplate.html for the source HTML file that is readable with instructions to update these strings.
===========================================================================*/

//=============================================================================================================================
//CTRL SITE PARTS - Used to serve up each device's controls
//WEBCTRLSTR is common to all devices and is REQUIRED
const char WEBCTRLSTR[] PROGMEM = "<!doctypehtml><title>fishDIY Limit-SW-Actuator</title><meta content=\"initial-scale=1\"name=viewport><script src=/CommonWebFunctions.js></script><link href=/styles.css rel=stylesheet id=styles><link href=styles.css rel=stylesheet><script src=CommonWebFunctions.js></script><div class=main id=myBody><script>function dealWithMessagePayload(e){var t=getMsg(e);console.log(t),setOtherTxt(t);var n=getNodeJSONtext(e);console.log(n),processJSON(n)}function setMainTxt(e){document.getElementById(\"mainTxt\").innerHTML=e+\"%\"}function setOtherTxt(e){document.getElementById(\"otherTxt\").innerHTML=e}function syncPO(){var e=document.getElementById(\"pctOpenSld\");document.getElementById(\"pobtntxt\").innerHTML=e.value+\"%\"}function openIsCCWUpd(){var e=document.getElementById(\"swLab\");1==document.getElementById(\"swChck\").checked?e.innerHTML=\"Open CCW\":e.innerHTML=\"Open CW\"}function swSwapUpd(){var e=document.getElementById(\"swSwapLab\");1==document.getElementById(\"swSwapChck\").checked?e.innerHTML=\"Swapped Lim Sw\":e.innerHTML=\"Normal Lim Sw\"}function sendCmd(e){if(1!=websock.readyState)return alert(\"This control panel is no longer connected to the device.  Please close this window and reopen the control panel.\"),0;var t=\"\";if(\"O\"==e)t=\"open\";else if(\"C\"==e)t=\"close\";else if(\"S\"==e)t=\"stop\";else if(\"cal\"==e)t=\"calibrate\";else if(\"reboot\"==e)t=\"reset\";else if(\"rstWifi\"==e)t=\"reset_wifi\";else if(\"G\"==e){var n;for(t=\"goto\",n=document.getElementById(\"pctOpenSld\").value;n.length<3;)n=\"0\"+n;t+=n}else if(\"config\"==e){var d=\"false\",o=\"false\",c=\"false\";document.getElementById(\"swMstrChck\").checked&&(d=\"true\"),document.getElementById(\"swChck\").checked&&(o=\"true\"),document.getElementById(\"swSwapChck\").checked&&(c=\"true\"),t=\"config\",t+=\";openIsCCW=\"+o,t+=\";isMaster=\"+d,t+=\";devName=\"+(\"\"==document.getElementById(\"devName\").value?\" \":document.getElementById(\"devName\").value),t+=\";note=\"+(\"\"==document.getElementById(\"devNote\").value?\" \":document.getElementById(\"devNote\").value),t+=\";swapLimSW=\"+c,t+=\";timeOut=\"+(\"\"==document.getElementById(\"timeOut\").value?\"60\":document.getElementById(\"timeOut\").value)}document.body.style.backgroundColor=\"var(--bg-col-ack)\",websock.send(t)}var infoText=\"...Loading...\";function doStuffWithJSON(e){var t=\"\",n=\"\",o=!1,c=\"\";d=e[0],returnColor(),\"true\"==d.isMaster?(t=\" (MASTER)\",document.getElementById(\"swMstrChck\").checked=!0):(t=\"\",document.getElementById(\"swMstrChck\").checked=!1),\"true\"==d.openIsCCW?document.getElementById(\"swChck\").checked=!0:(chckTxt=\"\",document.getElementById(\"swChck\").checked=!1),\"true\"==d.swapLimSW?document.getElementById(\"swSwapChck\").checked=!0:document.getElementById(\"swSwapChck\").checked=!1,n=\"true\"==d.motorPosAtCWset&&\"true\"==d.motorPosAtCCWset?\"\":(o=!0,c=\"UNCALIBRATED\\n\",\"**HARDWARE LIMITS ARE UNCALIBRATED**\\n(AUTOCAL IS RECOMMENDED)\\n\"),\"true\"==d.deviceTimedOut&&(o=!0,c+=\"ERROR - DEVICE TIMED OUT\\n\"),openIsCCWUpd(),swMstrUpd(),swSwapUpd(),infoText=\"DEVICE TYPE: \"+d.devType+\"<br>\\nNAME: \"+d.deviceName+\"<br>\\nIP ADDRESS: \"+d.ip+t+\"<hr>\\nMOTOR POSITION: \"+d.motorPos+\"<br>\\nMOTOR HARDWARE LIMITS: CCW=0 CW= \"+d.range+\"<hr>\\nSOFTWARE VERSION: \"+d.swVer+\"<br>\\nINIT STR: \"+d.initStamp+\"\\n\\n\\n<hr>\"+n+\"<br>\\n\"+c,document.getElementById(\"info-icon\").className=o?\"info-icon-blink\":\"info-icon\",setMainTxt(posPct(d.motorPos,d.range,d.openIsCCW)),document.getElementById(\"devIP\").value=d.ip+t,document.getElementById(\"devNote\").value=d.note,document.getElementById(\"timeOut\").value=d.timeOut,document.getElementById(\"devName\").value=d.deviceName,document.getElementById(\"devType\").value=d.devType}function posPct(e,t,n){if(0==t)return 0;var d=Math.round(e/t*100);return\"true\"==n&&(d=100-d),d}window.onload=start()</script></div><div class=flex-grid-outer id=flex-container><div class=fishyHdr id=deviceStatus><table width=100%><tr><td><div class=info-icon id=info-icon onclick=showDetails()>&#9432;</div><td><div class=flex-grid style=padding:6px><table><tr><td style=font-size:12px>Position:<tr><td id=mainTxt>100%</table><span class=otherTxt id=otherTxt></span></div></table></div><div><div class=buttonSet><div class=flex-grid><div class=sliderDiv><input id=pctOpenSld class=slider value=50 type=range max=100 min=0 oninput=syncPO()></div><div class=button onclick='sendCmd(\"G\")'><table width=100%><tr><td class=lilButtonTxt>GOTO<tr><td class=buttonTxt><span id=pobtntxt>50%</span></table></div></div><div class=flex-grid><div class=button onclick='sendCmd(\"O\")'><span class=buttonTxt>OPEN</span></div><div class=button onclick='sendCmd(\"S\")'><span class=buttonTxt>STOP</span></div><div class=button onclick='sendCmd(\"C\")'><span class=buttonTxt>CLOSE</span></div></div></div><div class=swPanel><div class=flex-grid><div class=swRow><label class=sw id=sw><input id=swChck type=checkbox onchange=openIsCCWUpd()><span class=sldSw></span></label><span class=swLab id=swLab>Open CW</span></div><div class=swRow><label class=sw id=swMstr><input id=swMstrChck type=checkbox onchange=swMstrUpd() checked><span class=sldSw></span></label><span class=swLab id=swMstrLab>Master Node</span></div><div class=swRow><label class=sw id=swSwap><input id=swSwapChck type=checkbox onchange=swSwapUpd() checked><span class=sldSw></span></label><span class=swLab id=swSwapLab>Swapped Lim SW</span></div></div></div></div><div class=configPanel><label class=cfgInpLab for=devIP>IP Address:<input id=devIP class=cfgInp value=...Loading... maxlength=40 disabled></label><br><label class=cfgInpLab for=devName>Name:<input id=devName class=cfgInp value=...Loading... maxlength=40 onkeypress=\"return blockSpecialChar(event)\"></label><br><label class=cfgInpLab for=devType>Type:<input id=devType class=cfgInp value=...Loading... maxlength=20 disabled></label><br><label class=cfgInpLab for=devNote>Note:<input id=devNote class=cfgInp value=...Loading... maxlength=55 onkeypress=\"return blockSpecialChar(event)\"></label><br><label class=cfgInpLab for=timeOut>Time-Out:<input id=timeOut class=cfgInpNum value=00 maxlength=5 onkeypress=\"return numberCharOnly(event)\"title=\"Enter a time (in seconds) to wait for the actuator to respond before stopped and displaying an error message (assuming the actuator got stuck).\"></label></div><div class=fishyFtrSw><div class=flex-grid><input id=updCfgBtn class=cfgbuttonY value=\"UPDATE SETTINGS\"title=\"This will save new settings in the device. New settings should be displayed on next refresh.  If trying to adjust mulitple settings it is recommended that you temporarily turn off Auto-refresh.\"onclick='sendCmd(\"config\")'type=button> <input id=updCfgBtn class=cfgbuttonY value=\"REBOOT DEVICE\"title=\"This will reboot the device.\"onclick='sendCmd(\"reboot\")'type=button></div><div class=flex-grid><input id=updCfgBtn class=cfgbuttonR value=\"AUTO-CAL. HW LIM\"title=\"WARNING:Auto-Cal will attempt to cycle your actuator full open and shut to determine  hardware limit switch positions. Ensure that range is possible to prevent damage.\"onclick='sendCmd(\"cal\")'type=button> <input id=rstWifiBtn class=cfgbuttonR value=\"RESET WIFI\"title=\"WARNING:Reset Wifi will attempt to delete your network SSID and passwordand put the device into wifi-server mode to learn new wifi SSID and password. To teach a reset device a new wifi network go to the device IP address using a mobile phone or computer.\"onclick='sendCmd(\"rstWifi\")'type=button></div></div></div><div class=infoPanel id=infoPanel><div class=infoDiv id=infoDiv></div><input class=infoOKbtn value=OK type=button onclick=closeCtrlModal()></div><script>var ctrlModal=document.getElementById(\"infoPanel\");function closeCtrlModal(){ctrlModal.style.display=\"none\"}</script>";