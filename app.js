let wasmExports;

async function loadWasm() {
  if (typeof WebAssembly === "undefined") {
    document.getElementById("output").innerHTML =
      "<p><strong>WebAssembly is not supported in this browser.</strong></p>";
    return;
  }

  try {
    const response = await fetch("genpass.wasm");

    const importObject = {
      wasi_snapshot_preview1: {
        proc_exit: (code) => {
          throw new WebAssembly.RuntimeError("WASM exited with code " + code);
        },
        random_get: (bufPtr, bufLen) => {
          const mem = wasmExports.memory.buffer; // always fresh
          crypto.getRandomValues(new Uint8Array(mem, bufPtr, bufLen));
          return 0;
        },
        clock_time_get: (clockId, precision, resultPtr) => {
          // clockId: which clock (0 = realtime, 1 = monotonic, etc.)
          // precision: requested precision (ignored here)
          // resultPtr: pointer to a 64-bit integer where we store nanoseconds

          let ns;
          if (clockId === 0) {
            // CLOCK_REALTIME: wall clock time since epoch
            ns = BigInt(Date.now()) * 1_000_000n;
          } else {
            // CLOCK_MONOTONIC: monotonic time since page load
            ns = BigInt(Math.floor(performance.now() * 1_000_000));
          }

          const view = new DataView(wasmExports.memory.buffer);
          view.setBigUint64(resultPtr, ns, true); // little-endian

          return 0; // success
        }
      }
    };

    const { instance } = await WebAssembly.instantiateStreaming(response, importObject);

    wasmExports = instance.exports;

    // Always call constructors immediately after instantiation
    wasmExports.__wasm_call_ctors();

    document.getElementById("output").innerHTML =
      "<p>WASM module loaded. Ready to generate.</p>";

  } catch (err) {
    console.error("Failed to load WASM:", err);
    document.getElementById("output").innerHTML =
      "<p><strong>Failed to load WebAssembly module.</strong></p>";
  }
}

function getCategoryEnum() {
  const selected = document.querySelector('input[name="category"]:checked');

  if (!selected) {
    // No radio selected → default to password
    return 0;
  }

  switch (selected.value) {
    case "username": return 0;
    case "password": return 1;
    case "passwordspecial": return 2;
    case "pin4": return 3;
    case "pin6": return 4;
    case "pin12": return 5;
    default:
      // Explicitly throw if value is unexpected
      throw new Error(`Unknown category value: ${selected.value}`);
  }
}

loadWasm();

const countinput = document.getElementById("count");

// Validate and sanitize the "count" input value
function getValidatedCount() {
  const input = document.getElementById("count");

  // Default fallback comes from the HTML attribute "value"
  const defaultCount = parseInt(input.getAttribute("value"), 10);

  // Parse current user input
  let value = parseInt(input.value, 10);

  // Ensure the value is an integer within [1, 4294967295]
  if (Number.isInteger(value)) {
    const mn = parseInt(input.getAttribute("min"), 10);
    const mx = parseInt(input.getAttribute("max"), 10);

    if (value < mn) {
      value = mn;
      countinput.value = String(value);
    } else if (value > mx) {
      value = mx;
      countinput.value = String(value);
    }
  } else {
    value = defaultCount;
    // Reset the input field to the corrected value
    countinput.value = String(value);
  }

  return value;
}

document.getElementById("generateBtn").addEventListener("click", () => {
  if (!wasmExports) return;

  const count = getValidatedCount();

  const categoryEnum = getCategoryEnum();

  wasmExports.generate_data(categoryEnum, count);

  const ptr = wasmExports.get_html_ptr();
  const len = wasmExports.get_html_len();
  const bytes = new Uint8Array(wasmExports.memory.buffer, ptr, len);
  const html = new TextDecoder("utf-8").decode(bytes);


  const durationInfo = document.getElementById("durationInfo");
  if (durationInfo && wasmExports) {
    // Get pointer and length for the formatted elapsed time string
    const ptr = wasmExports.generated_elapsed_time_ptr();
    const len = wasmExports.generated_elapsed_time_len();

    // Read the UTF-8 string from WASM memory
    const bytes = new Uint8Array(wasmExports.memory.buffer, ptr, len);
    const elapsedTime = new TextDecoder("utf-8").decode(bytes);

    // Show and update the DOM element
    durationInfo.style.display = "block";
    durationInfo.textContent = elapsedTime;
  }

  requestAnimationFrame(() => {
    document.getElementById("output").innerHTML = html;
  });
});

document.getElementById("saveBtn").addEventListener("click", () => {
  if (!wasmExports) return;

  const ptr = wasmExports.get_saved_ptr();
  const len = wasmExports.get_saved_len();
  const bytes = new Uint8Array(wasmExports.memory.buffer, ptr, len);
  const savedData = new TextDecoder("utf-8").decode(bytes);

  // Do not save if empty or whitespace only
  if (!savedData || savedData.trim().length === 0) {
    document.getElementById("output").innerHTML =
      "<p><strong>No data available to save.</strong></p>";
    return;
  }

  // Determine base name based on last generated category
  const category = wasmExports.get_last_generated_category();
  let baseName = "data";
  switch (category) {
    case 0: // password
      baseName = "password";
      break;
    case 1: // passwordspecial
      baseName = "passwordspecial";
      break;
    case 2: // username
      baseName = "username";
      break;
    case 3: // pin
      baseName = "pin4";
      break;
    case 4: // pin6
      baseName = "pin6";
      break;
    case 5: // pin12
      baseName = "pin12";
      break;
    default:
      baseName = "data";
      break;
  }

  // Get the formatted timestamp string from WASM
  const tsPtr = wasmExports.generated_last_timestamp_ptr();
  const tsLen = wasmExports.generated_last_timestamp_len();
  const tsBytes = new Uint8Array(wasmExports.memory.buffer, tsPtr, tsLen);
  const timestampStr = new TextDecoder("utf-8").decode(tsBytes);

  // Combine category name and timestamp into final filename
  const filename = `${baseName}_${timestampStr}.txt`;

  const blob = new Blob([savedData], { type: "text/plain" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  a.style.display = "none";
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);
});

document.addEventListener('DOMContentLoaded', () => {
  const backToTopBtn = document.getElementById('backToTop');

  // If the button does not exist, stop here
  if (!backToTopBtn) return;

  // 🔁 Show or hide the button when scrolling
  window.addEventListener('scroll', () => {
    if (window.scrollY > 300) {
      backToTopBtn.style.display = 'block';
    } else {
      backToTopBtn.style.display = 'none';
    }
  });

  // ⬆️ Scroll smoothly to the top when clicked
  backToTopBtn.addEventListener('click', () => {
    window.scrollTo({
      top: 0,
      behavior: 'smooth'
    });
  });
});

// Validate the input value whenever it changes
countinput.addEventListener("change", () => {
  // Always sanitize the value using your validation function
  getValidatedCount();
});

// Handle Enter key explicitly
countinput.addEventListener("keydown", (event) => {
  if (event.key === "Enter") {
    // Sanitize the value immediately when Enter is pressed
    getValidatedCount();
    // Optionally prevent form submission if inside a <form>
    event.preventDefault();
  }
});
