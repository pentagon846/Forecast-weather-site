# Use Dockerfile syntax version 1.5 or newer
# syntax=docker/dockerfile:1.5

# Specify Python version via an argument
ARG PYTHON_VERSION=3.11.8

# Use the slim version of the specified Python version
FROM python:${PYTHON_VERSION}-slim

# Label to identify the runtime environment
LABEL fly_launch_runtime="flask"

# Set working directory inside the container
WORKDIR /code

# Install system dependencies for Python packages if necessary
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential gcc \
    && rm -rf /var/lib/apt/lists/*

# Copy the requirements file and install Python dependencies
COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

# COPY .env .env
# Copy all remaining code to the working directory
COPY . .

# Expose port 8080 for Flask to run
EXPOSE 8080

# Default command to run the Flask application
CMD ["python3", "-m", "flask", "run", "--host=0.0.0.0", "--port=8080"]

