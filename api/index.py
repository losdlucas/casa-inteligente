
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BACKEND = os.path.join(ROOT, "backend")

sys.path.insert(0, ROOT)
sys.path.insert(0, BACKEND)

from app import app