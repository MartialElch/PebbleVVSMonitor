/******/ (function(modules) { // webpackBootstrap
/******/ 	// The module cache
/******/ 	var installedModules = {};
/******/
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/
/******/ 		// Check if module is in cache
/******/ 		if(installedModules[moduleId])
/******/ 			return installedModules[moduleId].exports;
/******/
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = installedModules[moduleId] = {
/******/ 			exports: {},
/******/ 			id: moduleId,
/******/ 			loaded: false
/******/ 		};
/******/
/******/ 		// Execute the module function
/******/ 		modules[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/
/******/ 		// Flag the module as loaded
/******/ 		module.loaded = true;
/******/
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/
/******/
/******/ 	// expose the modules object (__webpack_modules__)
/******/ 	__webpack_require__.m = modules;
/******/
/******/ 	// expose the module cache
/******/ 	__webpack_require__.c = installedModules;
/******/
/******/ 	// __webpack_public_path__
/******/ 	__webpack_require__.p = "";
/******/
/******/ 	// Load entry module and return exports
/******/ 	return __webpack_require__(0);
/******/ })
/************************************************************************/
/******/ ([
/* 0 */
/***/ (function(module, exports, __webpack_require__) {

	__webpack_require__(1);
	module.exports = __webpack_require__(2);


/***/ }),
/* 1 */
/***/ (function(module, exports) {

	(function(p) {
	  if (!p === undefined) {
	    console.error('Pebble object not found!?');
	    return;
	  }
	
	  // Aliases:
	  p.on = p.addEventListener;
	  p.off = p.removeEventListener;
	
	  // For Android (WebView-based) pkjs, print stacktrace for uncaught errors:
	  if (typeof window !== 'undefined' && window.addEventListener) {
	    window.addEventListener('error', function(event) {
	      if (event.error && event.error.stack) {
	        console.error('' + event.error + '\n' + event.error.stack);
	      }
	    });
	  }
	
	})(Pebble);


/***/ }),
/* 2 */
/***/ (function(module, exports) {

	function getDeparture(stopID, LineID) {
	  var req = new XMLHttpRequest();
	  var response;
	
	  console.log("get departure id = " + stopID + " line id = " + LineID);
	
	  var url = 'http://www2.vvs.de/vvs/XML_DM_REQUEST?outputFormat=JSON&sessionID=0&depType=stopEvents&mode=direct&limit=1&mergeDep=1&useRealtime=1&line=' + LineID + '&type_dm=name&name_dm='+ stopID;
	  method = 'GET';
	  req.onload = function() {
	    if (req.status == 200) {
	      response = JSON.parse(req.responseText);
	
	      var n = 0;
	      if (response.departureList) {
	        n = response.departureList.length;
	      }
	
	      // send to pebble
	      var message = {
	        "FuncDeparture": 1,
	        "Count": n,
	        "Line": response.departureList[0].servingLine.number,
	        "Direction": response.departureList[0].servingLine.direction,
	        "Countdown": response.departureList[0].countdown,
	      };
	
	      Pebble.sendAppMessage(message,
	        function(e) {
	          console.log("monitor list send success");
	        },
	        function(e) {
	          console.log("monitor list send failed");
	        }
	      );
	    }
	  };
	  req.open(method, url);
	  req.send();
	
	}
	
	function getDepartureList(stopID) {
	  var req = new XMLHttpRequest();
	  var response;
	
	  var StationLines = localStorage.getItem("StationLines");
	
	  var url = 'http://www2.vvs.de/vvs/XML_DM_REQUEST?outputFormat=JSON&sessionID=0&depType=stopEvents&mode=direct&limit=10&mergeDep=1&useRealtime=1&' + StationLines + 'type_dm=name&name_dm='+ stopID;
	  method = 'GET';
	  req.onload = function() {
	    if (req.status == 200) {
	      response = JSON.parse(req.responseText);
	
	      var n = 0;
	      if (response.departureList) {
	        n = response.departureList.length;
	      }
	
	      // send to pebble
	      var message = {
	        "FuncDepartureList": 1,
	        "Count": n,
	      };
	
	      message.Line = new Array(n);
	      message.Direction = new Array(n);
	      message.Countdown = new Array(n);
	      message.LineID = new Array(n);
	
	      for (var i=0; i<n; i++) {
	        message.Line[i] = 50 + i;
	        message.Direction[i] = 100 + i;
	        message.Countdown[i] = 150 + i;
	        message.LineID[i] = 200 + i;
	
	        message[50 + i] = response.departureList[i].servingLine.number;
	        message[100 + i] = response.departureList[i].servingLine.direction;
	        message[150 + i] = response.departureList[i].countdown;
	        message[200 + i] = response.departureList[i].servingLine.stateless.replace(/ /g, '');;
	      }
	
	      Pebble.sendAppMessage(message,
	        function(e) {
	          console.log("monitor list send success");
	        },
	        function(e) {
	          console.log("monitor list send failed");
	        }
	      );
	    }
	  };
	  req.open(method, url);
	  req.send();
	}
	
	// get lines serving station
	function getServingLines(stopID, product) {
	  var req = new XMLHttpRequest();
	
	  var url = 'http://www2.vvs.de/vvs/XML_DM_REQUEST?outputFormat=JSON&sessionID=0&depType=stopEvents&mode=direct&limit=0&mergeDep=1&useRealtime=1&line=any&type_dm=stationID&name_dm=' + stopID;
	  method = 'GET';
	  req.onload = function() {
	    if (req.status == 200) {
	      var response = JSON.parse(req.responseText);
	
	      var n = response.servingLines.lines.length;
	      var lines = [];
	      for (var i=0; i<n; i++) {
	        console.log("productId = " + response.servingLines.lines[i].mode.productId + " " + response.servingLines.lines[i].mode.product);
	        var productId = response.servingLines.lines[i].mode.productId;
	        if (product & (1 << productId)) {
	          lines.push(response.servingLines.lines[i].mode.diva.stateless);
	        }
	      }
	      lines.push(lines[0]);
	      lines = lines.filter(Unique);
	      stopLines = "";
	      for (var i=0; i<lines.length; i++) {
	        stopLines = stopLines + "line=" + lines[i] + "&";
	      }
	      stopLines = stopLines.replace(/ /g, '');
	
	      localStorage.setItem("StationID", stopID);
	      localStorage.setItem("StationLines", stopLines);
	
	      // send to pebble
	      var message = {
	        'FuncServingLines': 1,
	      };
	      Pebble.sendAppMessage(message,
	        function(e) {
	          console.log("lines send success");
	        },
	        function(e) {
	          console.log("lines send failed");
	        }
	      );
	    }
	  };
	  req.open(method, url);
	  req.send();
	}
	
	function getStationList(position) {
	  var req = new XMLHttpRequest();
	  var lat = position.coords.latitude;
	  var lon = position.coords.longitude;
	  var response;
	
	  var MAX = 10;
	  var DISTANCE = 5000;
	
	  console.log("DEBUG: lat = " + lat);
	  console.log("DEBUG: lon = " + lon);
	
	  // test code
	  lat = 48.775028;
	  lon = 9.171567;
	
	  // get list of nearby stations stations
	  var url = 'http://www2.vvs.de/vvs/XML_COORD_REQUEST?outputFormat=JSON&coordOutputFormat=WGS84&max=' + MAX + '&inclFilter=1&radius_1=' + DISTANCE + '&type_1=STOP&coord=' + lon + ':' + lat + ':WGS84';
	  method = 'GET';
	
	  req.onload = function() {
	    if (req.status == 200) {
	      response = JSON.parse(req.responseText);
	      var n = response.pins.length;
	
	      // send to pebble
	      var message = {
	        "FuncStationList": 1,
	        "Count": n,
	      };
	
	      message.StationName = new Array(n);
	      message.StationID = new Array(n);
	
	      for (var i=0; i<n; i++) {
	        message.StationName[i] = 100 + i;
	        message.StationID[i] = 150 + i;
	
	        message[100 + i] = response.pins[i].desc;
	        message[150 + i] = response.pins[i].id;
	      }
	
	      Pebble.sendAppMessage(message,
	        function(e) {
	          console.log("station list send success");
	        },
	        function(e) {
	          console.log("station list send failed");
	        }
	      );
	    }
	  };
	
	  req.open(method, url);
	  req.send();
	}
	
	// get GPS coordinates of current location
	function getLocation() {
	  navigator.geolocation.getCurrentPosition(getStationList);
	}
	
	Pebble.addEventListener("appmessage",
	  function(e) {
	    if (e.payload.FuncStationList == 1) {
	      getLocation();
	    } else if (e.payload.FuncDeparture == 1) {
	      getDeparture(e.payload.StationID, e.payload.LineID);
	    } else if (e.payload.FuncDepartureList == 1) {
	      getDepartureList(e.payload.StationID);
	    } else if (e.payload.FuncServingLines == 1) {
	      getServingLines(e.payload.StationID, e.payload.Product);
	    }
	  }
	);
	
	Pebble.addEventListener("ready",
	  function(e) {
	    console.log("PebbleKit JS ready!");
	    // send to pebble
	    var message = {
	      "Ready": 1,
	    };
	    Pebble.sendAppMessage(message,
	      function(e) {
	        console.log("ready send success");
	      },
	      function(e) {
	        console.log("ready send failed");
	      }
	    );
	  }
	);
	
	function Unique(value, index, self) {
	  return self.indexOf(value) === index;
	}


/***/ })
/******/ ]);
//# sourceMappingURL=pebble-js-app.js.map