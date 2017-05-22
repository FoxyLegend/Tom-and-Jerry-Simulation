var express = require('express');
var firebase = require('firebase');

var app = express();

app.use(express.static('public'));
app.get('/', function (req, res) {
	res.sendFile(__dirname + '/public/login.html');
});

app.listen(3000, function () {
  console.log('Example app listening on port 3000!');
});

/*
setInterval(function(){
	admin.auth().getUserByEmail('hsm6911@gmail.com')
	  .then(function(userRecord) {
		// See the tables above for the contents of userRecord
		console.log("Successfully fetched user data:", userRecord.toJSON());
	  })
	  .catch(function(error) {
		console.log("Error fetching user data:", error);
	  });
	
}, 1000);
*/