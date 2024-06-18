import javax.xml.parsers.DocumentBuilderFactory
import javax.xml.transform.TransformerFactory
import javax.xml.transform.dom.DOMSource
import javax.xml.transform.stream.StreamResult

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}
fun mergeManifestXmlFiles(mainXmlPath: String, libraryXmlPaths: List<String>): String {
    val mainManifestFile = file(mainXmlPath)
    val targetFile = File(mainManifestFile.getParent(), "AndroidManifest_target.xml")

    //从DOM工厂中获得DOM解析器
    val documentBuilder = DocumentBuilderFactory.newInstance().newDocumentBuilder();

    val mainXmlDocument = documentBuilder.parse(mainManifestFile)
    val mainApplicationNode = mainXmlDocument.getElementsByTagName("application").item(0);

    for (libraryXmlPath in libraryXmlPaths) {
        val libraryXmlDocument = documentBuilder.parse(file(libraryXmlPath))

        val libraryUsersPermissionList = libraryXmlDocument.getElementsByTagName("uses-permission")
        val length = libraryUsersPermissionList?.length ?: 0
        if (length > 0) {
            for (i in 0 until length) {
                val mainXmlNode = mainXmlDocument.getDocumentElement()
                mainXmlNode.insertBefore(mainXmlDocument.importNode(libraryUsersPermissionList.item(i), true), mainXmlNode.getFirstChild())
            }
        }

        val libraryApplication = libraryXmlDocument.getElementsByTagName("application").item(0)
        if (libraryApplication != null && libraryApplication.hasChildNodes()) {
            val children = libraryApplication.getChildNodes()
            for (i in 0 until children.length) {
                mainApplicationNode.appendChild(mainXmlDocument.importNode(children.item(i), true))
            }
        }
    }

    val transformer = TransformerFactory.newInstance().newTransformer();
    transformer.setOutputProperty("encoding", "utf-8")
    transformer.setOutputProperty("indent", "yes")
    val source = DOMSource(mainXmlDocument)
    val result = StreamResult(targetFile)
    transformer.transform(source, result)
    return targetFile.getAbsolutePath()
}
android {
    namespace = "com.linagzs.video"
    compileSdk = 33

    defaultConfig {
        applicationId = "com.linagzs.video"
        minSdk = 21
        targetSdk = 33
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }
    kotlinOptions {
        jvmTarget = "1.8"
    }
    buildFeatures {
        viewBinding = true
    }

    sourceSets {
        getByName("main") {
            java {
                srcDir("src/main/java")
                srcDir("../commonLib/src/main/java")
            }
            res {
                srcDir("src/main/res")
                srcDir("../commonLib/src/main/res")
            }
            assets {
                srcDir("src/main/assets")
                srcDir("../commonLib/src/main/assets")
            }

            manifest.srcFile(
                mergeManifestXmlFiles("src/main/AndroidManifest.xml", listOf<String>("../commonLib/src/main/AndroidManifest.xml"))
            )
        }
    }
}

dependencies {

    implementation("androidx.core:core-ktx:1.10.1")
    implementation("androidx.appcompat:appcompat:1.4.1")
    implementation("com.google.android.material:material:1.9.0")
    implementation("androidx.constraintlayout:constraintlayout:2.1.4")
    implementation("androidx.navigation:navigation-fragment-ktx:2.6.0")
    implementation("androidx.navigation:navigation-ui-ktx:2.6.0")
    testImplementation("junit:junit:4.13.2")
    androidTestImplementation("androidx.test.ext:junit:1.1.5")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.5.1")
}