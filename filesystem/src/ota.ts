import SparkMD5 from "spark-md5";

const fileInput = document.querySelector("input")!;
const fileName = document.getElementById("file-name")!;
const updateBtn = document.getElementById("update-btn")!;
const progressContainer = document.getElementById("progress-container")!;
const progressBar = document.getElementById("progress-bar")!;
const typeSelect = document.querySelector("select")!;

const feedback = document.createElement("div");
feedback.id = "feedback";
feedback.style.gridColumn = "span 2";
feedback.style.marginTop = "1em";
feedback.style.fontSize = "0.9em";
document.querySelector(".upload-container")!.appendChild(feedback);

fileInput.addEventListener("change", () => {
    progressBar.style.width = "0%";
    if (fileInput.files!.length > 0) {
        fileName.textContent = fileInput.files![0].name;
        updateBtn.removeAttribute("disabled");
        feedback.textContent = "";
    } else {
        fileName.textContent = "No file selected";
        updateBtn.setAttribute("disabled", "true");
    }
});

updateBtn.addEventListener("click", async () => {
    if (!fileInput.files!.length) return alert("Please select a file first.");

    const file = fileInput.files![0];
    const arrayBuffer = await file.arrayBuffer();
    const md5 = SparkMD5.ArrayBuffer.hash(arrayBuffer);
    const type = typeSelect.value;

    const url = `/update?name=${type}&md5=${md5}`;

    const xhr = new XMLHttpRequest();
    xhr.open("POST", url);
    xhr.setRequestHeader("Content-Length", file.size.toString());

    xhr.upload.onprogress = (event) => {
        if (event.lengthComputable) {
            const percent = (event.loaded / event.total) * 100;
            progressBar.style.width = `${percent}%`;
        }
    };

    xhr.onloadstart = () => {
        progressContainer.style.display = "block";
        progressBar.style.width = "0%";
        updateBtn.setAttribute("disabled", "true");
        feedback.textContent = "Uploading...";
        feedback.style.color = "#333";
    };

    xhr.onload = () => {
        const response = xhr.responseText.trim();
        if (xhr.status >= 200 && xhr.status < 300) {
            feedback.textContent = `✅ Success: ${response}`;
            feedback.style.color = "green";
        } else {
            feedback.textContent = `❌ Error: ${response}`;
            feedback.style.color = "red";
        }
        updateBtn.removeAttribute("disabled");
    };

    xhr.onerror = () => {
        feedback.textContent = "❌ Upload failed.";
        feedback.style.color = "red";
        updateBtn.removeAttribute("disabled");
    };

    xhr.send(file);
});
