package com.liangzs.trim

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView

class PcmMurgeActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_pcm_murge)

        findViewById<TextView>(R.id.murge).setOnClickListener {

        }
    }
}