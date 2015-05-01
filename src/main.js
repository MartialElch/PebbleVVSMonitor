function getDepartures(stopID) {
	var req = new XMLHttpRequest();
	var url = 'http://www2.vvs.de/vvs/XML_DM_REQUEST?outputFormat=JSON&sessionID=0&depType=stopEvents&mode=direct&limit=10&mergeDep=1&useRealtime=1&line=any&type_dm=stationID&name_dm='+ stopID;
  var response;

  req.open('GET', url, true);

	req.onload = function(e) {
		if (req.status == 200) {
			response = JSON.parse(req.responseText);

			var num = 0;
			num = response.departureList.length;

      var name = "";
      var countdown = [];
      var direction = "";

      for (var i=0; i< num; i++) {
        name = name + response.departureList[i].servingLine.number + ':';
        countdown = countdown + response.departureList[i].countdown + ':';
        direction = direction + response.departureList[i].servingLine.direction + ':';
      }

      var message = {
        "KEY_UPDATE_MONITOR": 1,
        "KEY_COUNT": num,
        "KEY_NAME": name,
        "KEY_COUNTDOWN": countdown,
        "KEY_DIRECTION": direction
      };

      // send to Pebble
      Pebble.sendAppMessage(message,
        function(e) {
          console.log('Departure info sent to Pebble successfully!');
        },
        function(e) {
          console.log('Error sending departure info to Pebble!');
        }
      );
		}
	};

	req.onerror = function(e) {
	};

	req.send(null);
}

function getStationsGPS(position) {
  var MAX_STOPS = 1;
  var req = new XMLHttpRequest();
	var lat = position.coords.latitude;
	var lon = position.coords.longitude;

  var response;

  console.log('DEBUG: lat = ' + lat);
  console.log('DEBUG: lon = ' + lon);

  lat = 48.775028;
  lon = 9.171567;
  
  var url = 'http://www2.vvs.de/vvs/XML_COORD_REQUEST?outputFormat=JSON&coordOutputFormat=WGS84&max=' + MAX_STOPS + '&inclFilter=1&radius_1=1320&type_1=STOP&coord=' + lon + ':' + lat + ':WGS84';
	req.open('GET', url, true);

	req.onload = function(e) {
    console.log('DEBUG: received stations');
		if (req.status == 200) {
			response = JSON.parse(req.responseText);

      var message = {
        "KEY_UPDATE_STATION": 1,
        "KEY_STATION": response.pins[0].desc,
        "KEY_STATIONID": response.pins[0].id
      };
      // send to Pebble
      Pebble.sendAppMessage(message,
        function(e) {
          console.log('Station info sent to Pebble successfully!');
        },
        function(e) {
          console.log('Error sending station info to Pebble!');
        }
      );
      getDepartures(response.pins[0].id);
    }
  };

	req.onerror = function(e) {
	};

	req.send(null);

}

function getLocation() {
  navigator.geolocation.getCurrentPosition(getStationsGPS);
}

// listen for AppMessage from Pebble
Pebble.addEventListener("appmessage", function(e) {
  console.log('DEBUG: AppMessage received ' + JSON.stringify (e.payload));

  if (e.payload.KEY_UPDATE_STATION == 1) {
    getLocation();
  } else if (e.payload.KEY_UPDATE_MONITOR == 1) {
    getDepartures(e.payload.KEY_STATIONID);
  }
});

Pebble.addEventListener("ready", function(e) {
  console.log('PebbleKit JS ready!');
});
