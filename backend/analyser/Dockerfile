FROM python:3.10-slim
WORKDIR /app
COPY app.py .
RUN pip install fastapi uvicorn onnxruntime spacy
RUN python -m spacy download en_core_web_sm
CMD ["uvicorn", "app:app", "--host", "0.0.0.0", "--port", "8004"]
