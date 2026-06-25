document.addEventListener("DOMContentLoaded", function () {
    // 1. Format raw byte sizes into human-readable strings (KB/MB)
    document.querySelectorAll(".byte-size").forEach(function (el) {
        const bytes = parseInt(el.textContent, 10);
        if (isNaN(bytes)) return;

        if (bytes >= 1048576) {
            el.textContent = (bytes / 1048576).toFixed(2) + " MB";
        } else if (bytes >= 1024) {
            el.textContent = (bytes / 1024).toFixed(2) + " KB";
        } else {
            el.textContent = bytes + " Bytes";
        }
    });

    // 2. Client-side validation to prevent non-binary uploads
    const fileInput = document.getElementById("file");
    const uploadForm = document.querySelector("form");

    if (fileInput && uploadForm) {
        uploadForm.addEventListener("submit", function (event) {
            const filePath = fileInput.value;
            const allowedExtension = /(\.bin)$/i;

            if (filePath && !allowedExtension.exec(filePath)) {
                alert("Validation Error: Only '.bin' files are accepted as firmware payloads.");
                event.preventDefault();
                return false;
            }
        });
    }
});