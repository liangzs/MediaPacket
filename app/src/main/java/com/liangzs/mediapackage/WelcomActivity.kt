package com.liangzs.mediapackage

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Handler
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

class WelcomActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_welcom)

        lifecycleScope.launch {
            delay(1000)
            val intent = Intent(this@WelcomActivity, MainActivity::class.java)
            startActivity(intent)
            finish()
        }
    }
}