console.log("🚀 Test Bridge Starting...");

const express = require("express");
const app = express();
const PORT = 3001;

console.log("✅ Express loaded");

app.use(express.json());

app.get("/health", (req, res) => {
  res.json({ status: "ok", message: "Test bridge is working" });
});

app.listen(PORT, () => {
  console.log(`✅ Test bridge running on http://localhost:${PORT}`);
  console.log("🔍 Test with: curl http://localhost:3001/health");
});

console.log("📝 Server setup complete");