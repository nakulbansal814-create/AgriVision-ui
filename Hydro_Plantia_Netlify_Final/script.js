import { initializeApp } from "https://www.gstatic.com/firebasejs/11.0.2/firebase-app.js";
import { getDatabase, ref, onValue, set } from "https://www.gstatic.com/firebasejs/11.0.2/firebase-database.js";

const firebaseConfig = {
  apiKey: "AIzaSyD0f45kNnYukEUo1vxPKkTTadnZ8_3EY5U",
  authDomain: "agri-vision-4a713.firebaseapp.com",
  projectId: "agri-vision-4a713",
  storageBucket: "agri-vision-4a713.firebasestorage.app",
  messagingSenderId: "626926429394",
  appId: "1:626926429394:web:21eb51e6a29bc5f842390b",
  measurementId: "G-ZH9NLMZJKH"
};

const app = initializeApp(firebaseConfig);
const db = getDatabase(app);

const sensorRef = ref(db, "esp32");
const controlRef = ref(db, "servo_control");
const statusRef = ref(db, "servo_status");

const tempHistory = [];
const soilHistory = [];
const timeHistory = [];
const MAX_POINTS = 20;

const $ = id => document.getElementById(id);

function numeric(value, fallback = 0) {
  const n = Number(value);
  return Number.isFinite(n) ? n : fallback;
}

function setText(id, value) {
  const el = $(id);
  if (el) el.textContent = value;
}

function drawChart(canvasId, values, type) {
  const canvas = $(canvasId);
  if (!canvas) return;
  const rect = canvas.getBoundingClientRect();
  const dpr = window.devicePixelRatio || 1;
  canvas.width = Math.max(1, rect.width * dpr);
  canvas.height = Math.max(1, rect.height * dpr);
  const ctx = canvas.getContext("2d");
  ctx.scale(dpr, dpr);

  const w = rect.width, h = rect.height;
  ctx.clearRect(0,0,w,h);

  const pad = {l:34,r:8,t:10,b:30};
  const cw = w-pad.l-pad.r, ch = h-pad.t-pad.b;

  let min, max;
  if (type === "soil") { min=0; max=100; }
  else {
    const finite = values.filter(Number.isFinite);
    const center = finite.length ? finite.reduce((a,b)=>a+b,0)/finite.length : 0;
    min = Math.floor(center - 5);
    max = Math.ceil(center + 5);
    if (min === max) { min -= 1; max += 1; }
  }

  ctx.strokeStyle = "#e6ebf2";
  ctx.lineWidth = 1;
  ctx.font = "12px system-ui";
  ctx.fillStyle = "#68758a";
  for(let i=0;i<=5;i++){
    const y=pad.t+ch*i/5;
    ctx.beginPath();ctx.moveTo(pad.l,y);ctx.lineTo(pad.l+cw,y);ctx.stroke();
    const val=max-(max-min)*i/5;
    ctx.fillText(type==="soil"?val.toFixed(0):val.toFixed(1),4,y+4);
  }

  if (!values.length) return;

  ctx.strokeStyle = type==="soil" ? "#3296df" : "#329fe8";
  ctx.lineWidth = 3;
  ctx.beginPath();
  values.forEach((v,i)=>{
    const x=pad.l+(values.length===1?cw/2:cw*i/(values.length-1));
    const y=pad.t+ch*(max-v)/(max-min);
    if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);
  });
  ctx.stroke();

  if(type==="soil"){
    ctx.lineTo(pad.l+cw*(values.length===1?.5:1),pad.t+ch);
    ctx.lineTo(pad.l,pad.t+ch);
    ctx.closePath();
    ctx.globalAlpha=.28;
    ctx.fillStyle="#329fe8";
    ctx.fill();
    ctx.globalAlpha=1;
  }
}

function updateCharts() {
  drawChart("temperatureChart", tempHistory, "temp");
  drawChart("soilChart", soilHistory, "soil");
}

onValue(sensorRef, snapshot => {
  const d = snapshot.val();

  if (!d) {
    $("firebaseBadge").innerHTML = "<i></i> Firebase Connected";
    return;
  }

  const temp = numeric(d.temperature);
  const humidity = numeric(d.humidity);
  const soil = numeric(d.soil_moisture);
  const raw = numeric(d.soil_raw);
  const motion = d.motion_active === true;
  const count = numeric(d.motion_count);
  const dht = d.dht_status ?? "--";

  setText("temperatureValue", `${temp.toFixed(1)} °C`);
  setText("humidityValue", `${humidity.toFixed(1)} %`);
  setText("soilValue", `${soil.toFixed(0)} %`);
  setText("motionCount", `${count}`);

  setText("detailTemp", `${temp.toFixed(1)} °C`);
  setText("detailHumidity", `${humidity.toFixed(1)} %`);
  setText("detailSoilRaw", `${Math.round(raw)}`);
  setText("detailSoil", `${soil.toFixed(0)} %`);
  setText("detailMotion", motion ? "MOTION DETECTED" : "NORMAL");
  setText("detailDht", dht);

  const motionEl = $("motionStatus");
  if (motion) {
    motionEl.innerHTML = "<i></i> MOTION DETECTED";
    motionEl.style.color = "#d14b3f";
  } else {
    motionEl.innerHTML = "<i></i> SYSTEM SECURE";
    motionEl.style.color = "#009b70";
  }

  const now = new Date();
  timeHistory.push(now.toLocaleTimeString());
  tempHistory.push(temp);
  soilHistory.push(soil);
  if(timeHistory.length > MAX_POINTS){
    timeHistory.shift(); tempHistory.shift(); soilHistory.shift();
  }

  setText("lastUpdate", `Last sensor update: ${now.toLocaleTimeString()}`);
  $("firebaseBadge").innerHTML = "<i></i> Firebase Connected";
  updateCharts();
});

onValue(statusRef, snapshot => {
  const s = snapshot.val();
  if (!s) return;
  setText("servo1Status", s.servo1 === true ? "OPEN" : "CLOSED");
  setText("servo2Status", s.servo2 === true ? "OPEN" : "CLOSED");
});

function command(path, state) {
  set(ref(db, `servo_control/${path}`), state)
    .catch(err => {
      console.error("Firebase command failed:", err);
      alert("Could not send command to Firebase.");
    });
}

$("servo1Open").addEventListener("click", () => command("servo1", true));
$("servo1Close").addEventListener("click", () => command("servo1", false));
$("servo2Open").addEventListener("click", () => command("servo2", true));
$("servo2Close").addEventListener("click", () => command("servo2", false));
$("allOpen").addEventListener("click", () => {
  set(ref(db, "servo_control"), {servo1:true, servo2:true})
    .catch(err => alert("Could not deploy all zones."));
});
$("allClose").addEventListener("click", () => {
  set(ref(db, "servo_control"), {servo1:false, servo2:false})
    .catch(err => alert("Could not secure all zones."));
});

window.addEventListener("resize", updateCharts);
updateCharts();
