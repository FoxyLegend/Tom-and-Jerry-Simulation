var express = require('express');
const exec = require('child_process').exec;

var admin = require("firebase-admin");
var serviceAccount = require(__dirname + "/cs408-transmitter-hunting-firebase-adminsdk-pnji9-9e850c0c69.json");
admin.initializeApp({
  credential: admin.credential.cert(serviceAccount),
  databaseURL: "https://cs408-transmitter-hunting.firebaseio.com"
});

var app = express();
app.use(express.static('public'));
app.get('/', function (req, res) {
	res.sendFile(__dirname + '/public/login.html');
});

app.listen(3000, function () {
  console.log('Example app listening on port 3000!');
});

var db = admin.database();
var ref = db.ref("gameroom");
var calculate = false;
var members = null;
ref.on("value", function(snapshot) {
	val = snapshot.val();
	if(!val || val.state != 'started'){
		calculate = false;
		members = null;
	}
	else {
		calculate = true;
		members = val.members;
	}
});


var gx, gy;
var ax = [], ay = [], hid = [];
var signal;


var cx = 1273623389;
var cy = 363706170;

setInterval(function(){
	if(!calculate) return;
	var count = 0;
	var fox = 0;
	ax = [], ay = [], hid = [];
	for(var mid in members){
		var m = members[mid];
		//console.log(m);
		if(!m.email){
			ref.child("members").child(mid).remove();
		}
		else {
			if(m.lng && m.lat){				
				px = (parseInt(m.lng * 1e7) - cx) * 400 / 80000 + 400;
				py = -(parseInt(m.lat * 1e7) - cy) * 400 / 80000 + 400;
			}
			
			if(m.role == 'fox'){
				gx = px;
				gy = py;
				fox++;
			}
			if(m.role == 'hound'){
				ax.push(px);
				ay.push(py);
				hid.push(mid);
				count++;
			}
		}
	}
	
	if(fox != 1 || count < 1) return;
	signal = null;
	var prog = "SeqProgram.exe " + gx + " " + gy + " " + ax[0] + " " + ay[0];
	exec(prog, (err, stdout, stderr) => {
		if (err) {
			console.error(err);
			return;
		}
		//console.log(stdout);
		signal = JSON.parse(stdout);
		//console.log(signal);
		
		//console.log(hid[0], signal);
		ref.child("members").child(hid[0]).child("signal").set(JSON.stringify(signal));
		console.log("Newly calculated: ", (new Date()).toGMTString(), signal.length);
	});
	
	
}, 3000);
