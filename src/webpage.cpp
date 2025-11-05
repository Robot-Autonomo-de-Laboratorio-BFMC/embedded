const char *webpage = R"html(
<!DOCTYPE html>
<html lang="es">
  <head>
    <title>Control Remoto RC-CAR</title>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.css" rel="stylesheet" />
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/1.12.4/jquery.min.js"></script>
    <style>
      #range-slider { width: 90%; height: 20px; }
      .btn-animate { transition: transform 0.2s ease; }
      .btn-animate:active { transform: scale(0.9); }
      * { touch-action: manipulation; }
      #status-overlay {
        position: fixed; top: 10px; right: 10px;
        background: rgba(0,0,0,0.8); color: white;
        padding: 10px; border-radius: 5px; font-size: 12px;
        z-index: 1000; min-width: 150px;
      }
      .status-item { margin: 5px 0; }
      .status-label { font-weight: bold; }
    </style>
  </head>
  <body style="height: 100vh; background-color: white; display: flex; align-items: center; justify-content: center;">
    <!-- Status Overlay -->
    <div id="status-overlay">
      <div class="status-item">
        <span class="status-label">Mode:</span> <span id="status-mode">MANUAL</span>
      </div>
      <div class="status-item">
        <span class="status-label">State:</span> <span id="status-state">DISARMED</span>
      </div>
      <div class="status-item">
        <span class="status-label">Heartbeat:</span> <span id="status-heartbeat">--</span>ms
      </div>
    </div>

    <div class="cont" style="display: grid; grid-template-columns: repeat(3); grid-template-rows: repeat(7);">
      <!-- Mode Controls Row -->
      <div style="grid-row: 1; grid-column: 1 / span 3; text-align: center; display: flex; gap: 10px; justify-content: center; align-items: center;">
        <button class="btn btn-primary btn-sm" onclick="setMode('MANUAL')">MANUAL</button>
        <button class="btn btn-primary btn-sm" onclick="setMode('AUTO')">AUTO</button>
        <button class="btn btn-success btn-sm" onclick="makeAjaxCall('arm')">ARM</button>
        <button class="btn btn-warning btn-sm" onclick="makeAjaxCall('disarm')">DISARM</button>
        <button class="btn btn-danger btn-sm" onclick="makeAjaxCall('brake')">BRAKE</button>
      </div>

      <!--LightsOff-->
      <div style="grid-row: 2; grid-column: 1; text-align: center; margin-bottom: 10px;">
        <button title="lightOff" class="btn btn-warning btn-lg btn-animate" style="font-size: 1.5rem; padding: 20px 25px" ontouchstart='makeAjaxCall("LightsOff")'>
          <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" viewBox="0 0 16 16">
            <path d="M2 6a6 6 0 1 1 10.174 4.31c-.203.196-.359.4-.453.619l-.762 1.769A.5.5 0 0 1 10.5 13h-5a.5.5 0 0 1-.46-.302l-.761-1.77a1.964 1.964 0 0 0-.453-.618A5.984 5.984 0 0 1 2 6zm3 8.5a.5.5 0 0 1 .5-.5h5a.5.5 0 0 1 0 1l-.224.447a1 1 0 0 1-.894.553H6.618a1 1 0 0 1-.894-.553L5.5 15a.5.5 0 0 1-.5-.5z"/>
          </svg>
        </button>
      </div>
      <!--LightsOn-->
      <div style="grid-row: 2; grid-column: 2; text-align: center">
        <button title="lightOn" class="btn btn-warning btn-lg btn-animate" style="font-size: 1.5rem; padding: 20px 25px" ontouchstart='makeAjaxCall("LightsOn")'>
          <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" viewBox="0 0 16 16">
            <path d="M2 6a6 6 0 1 1 10.174 4.31c-.203.196-.359.4-.453.619l-.762 1.769A.5.5 0 0 1 10.5 13a.5.5 0 0 1 0 1 .5.5 0 0 1 0 1l-.224.447a1 1 0 0 1-.894.553H6.618a1 1 0 0 1-.894-.553L5.5 15a.5.5 0 0 1 0-1 .5.5 0 0 1 0-1 .5.5 0 0 1-.46-.302l-.761-1.77a1.964 1.964 0 0 0-.453-.618A5.984 5.984 0 0 1 2 6zm6-5a5 5 0 0 0-3.479 8.592c.263.254.514.564.676.941L5.83 12h4.342l.632-1.467c.162-.377.413-.687.676-.941A5 5 0 0 0 8 1z"/>
          </svg>
        </button>
      </div>
      <!--LightsAuto-->
      <div style="grid-row: 2; grid-column: 3; text-align: center">
        <button class="btn btn-warning btn-lg btn-animate" style="font-size: 1rem; padding: 26px 20px" ontouchstart='makeAjaxCall("LightsAuto")'>
          <label class="fw-bold">AUTO</label>
        </button>
      </div>

      <!--Velocity Slider-->
      <div style="grid-row: 3; grid-column: 1 / span 3; text-align: center">
        <h2>Velocidad</h2>
        <input type="range" min="60" max="255" value="255" id="range-slider" onchange='makeAjaxCall("changeSpeed?speed=" + this.value)' />
      </div>

      <!--Servo Controls (simplified)-->
      <div style="grid-row: 4; grid-column: 1 / span 3; text-align: center; padding: 5px">
        <h4>Servo Angles: Left 50° | Center 105° | Right 135°</h4>
      </div>

      <!--Forward-->
      <div style="grid-column: 2; grid-row: 5; text-align: center">
        <button id="drive" type="button" class="btn btn-dark btn-lg btn-animate" style="border-radius: 80px; font-size: 1.5rem; padding: 30px 30px" ontouchstart='makeAjaxCall("forward")' ontouchend='makeAjaxCall("driveStop")'>
          <svg xmlns="http://www.w3.org/2000/svg" width="50" height="50" fill="currentColor" viewBox="0 0 16 16">
            <path d="m7.247 4.86-4.796 5.481c-.566.647-.106 1.659.753 1.659h9.592a1 1 0 0 0 .753-1.659l-4.796-5.48a1 1 0 0 0-1.506 0z"/>
          </svg>
        </button>
      </div>

      <!--Reverse-->
      <div style="grid-column: 2; grid-row: 7; text-align: center">
        <button id="back" type="button" class="btn btn-dark btn-lg btn-animate" style="border-radius: 80px; font-size: 1.5rem; padding: 30px 30px" ontouchstart='makeAjaxCall("back")' ontouchend='makeAjaxCall("driveStop")'>
          <svg xmlns="http://www.w3.org/2000/svg" width="50" height="50" fill="currentColor" viewBox="0 0 16 16">
            <path d="M7.247 11.14 2.451 5.658C1.885 5.013 2.345 4 3.204 4h9.592a1 1 0 0 1 .753 1.659l-4.796 5.48a1 1 0 0 1-1.506 0z"/>
          </svg>
        </button>
      </div>

      <!--TurnLeft-->
      <div style="grid-row: 6; grid-column: 1; text-align: end">
        <button id="left" type="button" class="btn btn-dark btn-lg btn-animate" style="border-radius: 80px; font-size: 1.5rem; padding: 30px 30px" ontouchstart='makeAjaxCall("left")' ontouchend='makeAjaxCall("steerStop")'>
          <svg xmlns="http://www.w3.org/2000/svg" width="50" height="50" fill="currentColor" viewBox="0 0 16 16">
            <path d="m3.86 8.753 5.482 4.796c.646.566 1.658.106 1.658-.753V3.204a1 1 0 0 0-1.659-.753l-5.48 4.796a1 1 0 0 0 0 1.506z"/>
          </svg>
        </button>
      </div>

      <!--TurnRight-->
      <div style="grid-row: 6; grid-column: 3; text-align: start">
        <button id="right" type="button" class="btn btn-dark btn-lg btn-animate" style="border-radius: 80px; font-size: 1.5rem; padding: 30px 30px" ontouchstart='makeAjaxCall("right")' ontouchend='makeAjaxCall("steerStop")'>
          <svg xmlns="http://www.w3.org/2000/svg" width="50" height="50" fill="currentColor" viewBox="0 0 16 16">
            <path d="m12.14 8.753-5.482 4.796c-.646.566-1.658.106-1.658-.753V3.204a1 1 0 0 1 1.659-.753l5.48 4.796a1 1 0 0 1 0 1.506z"/>
          </svg>
        </button>
      </div>
    </div>

    <script>
      function makeAjaxCall(url) { $.ajax({ url: url }); }
      
      function setMode(mode) {
        makeAjaxCall('mode?value=' + mode);
        updateStatus();
      }
      
      function updateStatus() {
        $.ajax({
          url: '/status',
          success: function(data) {
            var status = JSON.parse(data);
            document.getElementById('status-mode').textContent = status.mode;
            document.getElementById('status-state').textContent = status.state;
          }
        });
      }
      
      // Update status every second
      setInterval(updateStatus, 1000);
      updateStatus();
      
      // Keyboard controls
      document.addEventListener("keydown", function(event) {
        if (event.keyCode == 37) makeAjaxCall("left");
        else if (event.keyCode == 39) makeAjaxCall("right");
        else if (event.keyCode == 38) makeAjaxCall("forward");
        else if (event.keyCode == 40) makeAjaxCall("back");
      });
      document.addEventListener("keyup", function(event) {
        if (event.keyCode == 37 || event.keyCode == 39) makeAjaxCall("steerStop");
        else if (event.keyCode == 38 || event.keyCode == 40) makeAjaxCall("driveStop");
      });
    </script>
  </body>
</html>
)html";



