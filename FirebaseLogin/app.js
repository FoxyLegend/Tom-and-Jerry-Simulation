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
ref.child("members").on("value", function(snapshot) {
	var val = snapshot.val();
	members = val;
});

var gx, gy;
var ax = [], ay = [], hid = [];
var signal;


var cx = 1273623389;
var cy = 363706170;


function calculateSignal(){
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
}

var timer;
var intervalID;
//starting the actual game
ref.child("state").on("value", function(snapshot){
	var val = snapshot.val();
	if(!val) return;
	if(val == "starting"){
		timer = 5;
		ref.child("time").set(timer);
		intervalID = setInterval(function(){
			if(timer > 0) timer--;
			
			ref.child("time").set(timer);
			if(timer == 0){
				ref.child("state").set("started");
				clearInterval(intervalID);
			}
		}, 1000);
	}
	if(val == "started"){
		timer = 60*30;
		ref.child("time").set(timer);
		
		intervalID = setInterval(function(){
			if(timer%3 == 0) calculateSignal();
			if(timer > 0) timer--;
			
			ref.child("time").set(timer);
			if(timer == 0){
				ref.child("state").set("finished");
				clearInterval(intervalID);
			}
		}, 1000);
	}
});

